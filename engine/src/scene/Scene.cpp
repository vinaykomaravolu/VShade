#include "scene/Scene.hpp"
#include "scene/SceneComponentRegistry.hpp"

#include <atomic>
#include <random>
#include <stdexcept>
#include <utility>

namespace vshade::scene {

namespace {

[[nodiscard]] std::uint64_t initialUuidValue() {
    std::random_device random;
    const std::uint64_t high = static_cast<std::uint64_t>(random()) << 32U;
    const std::uint64_t low = static_cast<std::uint64_t>(random());
    const std::uint64_t value = high | low;
    return value == 0 ? 1 : value;
}

std::atomic<std::uint64_t> nextUuid{initialUuidValue()};

} // namespace

Scene::Scene(std::string name)
    : m_name(std::move(name)) {}
Scene::~Scene() = default;

Entity Scene::createEntity(std::string name) {
    return createEntityWithUuid(std::move(name), generateUuid());
}

Entity Scene::createEntityWithUuid(std::string name, const std::uint64_t uuid) {
    if (uuid == 0 || m_entitiesByUuid.contains(uuid)) {
        throw std::invalid_argument("Entity UUID must be non-zero and unique within its scene");
    }
    const entt::entity handle = m_registry.create();
    try {
        m_registry.emplace<UUIDComponent>(handle, uuid);
        m_registry.emplace<TagComponent>(handle, std::move(name));
        m_registry.emplace<TransformComponent>(handle);
        m_entitiesByUuid.emplace(uuid, handle);
    } catch (...) {
        m_registry.destroy(handle);
        throw;
    }
    return Entity(handle, *this, m_generation);
}

Entity Scene::duplicateEntity(const Entity source) {
    if (!valid(source)) {
        throw std::invalid_argument("Cannot duplicate an invalid or foreign entity");
    }

    Entity duplicate = createEntity(source.component<TagComponent>().tag + " Copy");
    try {
        duplicate.component<TransformComponent>() = source.component<TransformComponent>();
        if (source.hasComponents<SpriteRendererComponent>()) {
            duplicate.addComponent<SpriteRendererComponent>(
                source.component<SpriteRendererComponent>()
            );
        }
        if (source.hasComponents<AudioSourceComponent>()) {
            duplicate.addComponent<AudioSourceComponent>(
                source.component<AudioSourceComponent>()
            );
        }
        if (source.hasComponents<AudioListenerComponent>()) {
            duplicate.addComponent<AudioListenerComponent>(
                source.component<AudioListenerComponent>()
            );
        }
        if (source.hasComponents<LightComponent>()) {
            duplicate.addComponent<LightComponent>(
                source.component<LightComponent>()
            );
        }
        if (source.hasComponents<ScriptComponent>()) {
            duplicate.addComponent<ScriptComponent>(
                source.component<ScriptComponent>()
            );
        }
        for (const auto& handler : SceneComponentRegistry::handlers()) {
            handler.clone(m_registry, source.m_handle, m_registry, duplicate.m_handle);
        }
    } catch (...) {
        destroyEntity(duplicate);
        throw;
    }
    return duplicate;
}

void Scene::destroyEntity(const Entity entity) {
    if (!valid(entity)) {
        throw std::invalid_argument("Cannot destroy an invalid or foreign entity");
    }
    m_entitiesByUuid.erase(entity.uuid());
    m_registry.destroy(entity.m_handle);
}

bool Scene::valid(const Entity entity) const noexcept {
    return entity.m_scene == this &&
        entity.m_sceneGeneration == m_generation &&
        m_registry.valid(entity.m_handle);
}

Entity Scene::findEntity(const std::uint64_t uuid) noexcept {
    const auto found = m_entitiesByUuid.find(uuid);
    if (found != m_entitiesByUuid.end() && m_registry.valid(found->second)) {
        return Entity(found->second, *this, m_generation);
    }
    return {};
}

const std::string& Scene::name() const noexcept {
    return m_name;
}

void Scene::setName(std::string name) {
    m_name = std::move(name);
}

const SceneEnvironment& Scene::environment() const noexcept {
    return m_environment;
}

SceneEnvironment& Scene::environment() noexcept {
    return m_environment;
}

void Scene::setEnvironment(const SceneEnvironment& environment) noexcept {
    m_environment = environment;
}

std::uint64_t Scene::generateUuid() {
    std::uint64_t candidate = 0;
    do {
        candidate = nextUuid.fetch_add(1, std::memory_order_relaxed);
    } while (candidate == 0 || findEntity(candidate));
    return candidate;
}

} // namespace vshade::scene
