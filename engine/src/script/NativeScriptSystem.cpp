#include "script/NativeScriptSystem.hpp"

#include "core/Log.hpp"
#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Scene.hpp"
#include "script/NativeScript.hpp"
#include "script/NativeScriptRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vshade::script {
namespace {

struct DesiredScript {
    scene::Entity entity;
    std::uint64_t entityUuid = 0;
    std::size_t bindingIndex = 0;
    std::string typeName;
};

struct ScriptInstance {
    std::uint64_t entityUuid = 0;
    std::size_t bindingIndex = 0;
    std::string typeName;
    std::unique_ptr<NativeScript> script;
};

[[nodiscard]] bool sameBinding(
    const ScriptInstance& instance,
    const DesiredScript& desired
) noexcept {
    return instance.entityUuid == desired.entityUuid &&
        instance.bindingIndex == desired.bindingIndex &&
        instance.typeName == desired.typeName;
}

void validateDeltaTime(const float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime < 0.0F) {
        throw std::invalid_argument("Script delta time must be finite and non-negative");
    }
}

} // namespace

struct NativeScriptSystem::Impl {
    explicit Impl(NativeScriptRegistry& initialRegistry) noexcept
        : registry(&initialRegistry) {}

    [[nodiscard]] std::vector<DesiredScript> desiredScripts() const {
        std::vector<DesiredScript> desired;
        auto view = attachedScene->view<
            const scene::UUIDComponent,
            const scene::ScriptComponent
        >();
        for (const auto [handle, uuid, component] : view.each()) {
            (void)handle;
            for (std::size_t index = 0; index < component.scripts.size(); ++index) {
                const scene::ScriptBinding& binding = component.scripts[index];
                if (binding.backend != ScriptBackend::NativeCpp) {
                    continue;
                }
                desired.push_back({
                    .entity = attachedScene->findEntity(uuid.uuid),
                    .entityUuid = uuid.uuid,
                    .bindingIndex = index,
                    .typeName = binding.typeName,
                });
            }
        }
        return desired;
    }

    void synchronize() {
        const std::vector<DesiredScript> desired = desiredScripts();

        for (std::size_t index = 0; index < instances.size();) {
            const bool retained = std::ranges::any_of(
                desired,
                [&instance = instances[index]](const DesiredScript& candidate) {
                    return sameBinding(instance, candidate);
                }
            );
            if (retained) {
                ++index;
                continue;
            }

            std::unique_ptr<NativeScript> removed = std::move(instances[index].script);
            instances.erase(instances.begin() + static_cast<std::ptrdiff_t>(index));
            removed->onDestroy();
        }

        for (const DesiredScript& desiredScript : desired) {
            const bool exists = std::ranges::any_of(
                instances,
                [&desiredScript](const ScriptInstance& instance) {
                    return sameBinding(instance, desiredScript);
                }
            );
            if (exists) {
                continue;
            }

            std::unique_ptr<NativeScript> script = registry->create(desiredScript.typeName);
            NativeScriptSystem::attachInstance(*script, desiredScript.entity);
            script->onCreate();
            instances.push_back({
                .entityUuid = desiredScript.entityUuid,
                .bindingIndex = desiredScript.bindingIndex,
                .typeName = desiredScript.typeName,
                .script = std::move(script),
            });
        }
    }

    [[nodiscard]] bool enabled(const ScriptInstance& instance) const {
        const scene::Entity entity = attachedScene->findEntity(instance.entityUuid);
        if (!entity || !entity.hasComponents<scene::ScriptComponent>()) {
            return false;
        }
        const auto& scripts = entity.component<scene::ScriptComponent>().scripts;
        if (instance.bindingIndex >= scripts.size()) {
            return false;
        }
        const scene::ScriptBinding& binding = scripts[instance.bindingIndex];
        return binding.backend == ScriptBackend::NativeCpp &&
            binding.typeName == instance.typeName && binding.enabled;
    }

    NativeScriptRegistry* registry = nullptr;
    scene::Scene* attachedScene = nullptr;
    std::vector<ScriptInstance> instances;
};

NativeScriptSystem::NativeScriptSystem(NativeScriptRegistry& registry)
    : m_impl(std::make_unique<Impl>(registry)) {}

NativeScriptSystem::~NativeScriptSystem() {
    detachScene();
}

NativeScriptSystem::NativeScriptSystem(NativeScriptSystem&&) noexcept = default;

NativeScriptSystem& NativeScriptSystem::operator=(NativeScriptSystem&& other) noexcept {
    if (this != &other) {
        detachScene();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

void NativeScriptSystem::attachInstance(
    NativeScript& script,
    const scene::Entity entity
) noexcept {
    script.attach(entity);
}

void NativeScriptSystem::attachScene(scene::Scene& scene) {
    if (!m_impl) {
        throw std::logic_error("Cannot use a moved-from NativeScriptSystem");
    }
    if (m_impl->attachedScene != nullptr) {
        throw std::logic_error("NativeScriptSystem already has an attached scene");
    }

    m_impl->attachedScene = &scene;
    try {
        m_impl->synchronize();
    } catch (...) {
        detachScene();
        throw;
    }
}

void NativeScriptSystem::update(const float deltaTime) {
    validateDeltaTime(deltaTime);
    if (!hasAttachedScene()) {
        throw std::logic_error("NativeScriptSystem requires an attached scene");
    }

    m_impl->synchronize();
    for (ScriptInstance& instance : m_impl->instances) {
        if (m_impl->enabled(instance)) {
            instance.script->onUpdate(deltaTime);
        }
    }
}

void NativeScriptSystem::fixedUpdate(const float fixedDeltaTime) {
    validateDeltaTime(fixedDeltaTime);
    if (!hasAttachedScene()) {
        throw std::logic_error("NativeScriptSystem requires an attached scene");
    }

    m_impl->synchronize();
    for (ScriptInstance& instance : m_impl->instances) {
        if (m_impl->enabled(instance)) {
            instance.script->onFixedUpdate(fixedDeltaTime);
        }
    }
}

void NativeScriptSystem::detachScene() noexcept {
    if (!m_impl) {
        return;
    }
    for (auto iterator = m_impl->instances.rbegin();
         iterator != m_impl->instances.rend(); ++iterator) {
        try {
            iterator->script->onDestroy();
        } catch (const std::exception& error) {
            ENGINE_ERROR("Native script onDestroy failed: {}", error.what());
        } catch (...) {
            ENGINE_ERROR("Native script onDestroy failed with an unknown error");
        }
    }
    m_impl->instances.clear();
    m_impl->attachedScene = nullptr;
}

bool NativeScriptSystem::hasAttachedScene() const noexcept {
    return m_impl && m_impl->attachedScene != nullptr;
}

std::size_t NativeScriptSystem::instanceCount() const noexcept {
    return m_impl ? m_impl->instances.size() : 0;
}

} // namespace vshade::script
