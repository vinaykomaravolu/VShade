#include "scene/SceneRuntime.hpp"

#include "core/Log.hpp"
#include "core/EngineServices.hpp"
#include "physics/physics2d/PhysicsSystem2D.hpp"
#include "physics/physics3d/PhysicsSystem3D.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneAudioSystem.hpp"
#include "scene/SceneRenderer.hpp"
#include "script/NativeScriptRegistry.hpp"
#include "script/NativeScriptSystem.hpp"

#include <stdexcept>
#include <array>
#include <vector>
#include <utility>

namespace vshade::scene {
namespace {

constexpr std::size_t phaseIndex(const SceneRuntimePhase phase) noexcept {
    return static_cast<std::size_t>(phase);
}

void clearPhysics(
    physics::PhysicsSystem2D& physics2D,
    physics::PhysicsSystem3D& physics3D
) noexcept {
    try {
        physics2D.clear();
    } catch (...) {
        ENGINE_ERROR("Failed to clear SceneRuntime 2D physics");
    }
    try {
        physics3D.clear();
    } catch (...) {
        ENGINE_ERROR("Failed to clear SceneRuntime 3D physics");
    }
}

} // namespace

struct SceneRuntime::Impl {
    explicit Impl(
        script::NativeScriptRegistry& registry,
        const SceneRuntimeConfig initialConfig = {}
    ) : nativeScripts(registry), config(initialConfig) {}

    Impl(core::EngineServices& services, const SceneRuntimeConfig initialConfig)
        : services(&services),
          nativeScripts(services.scripts()),
          renderer(initialConfig.rendering
              ? std::make_unique<SceneRenderer>(services.assets())
              : nullptr),
          audio(initialConfig.audio
              ? std::make_unique<SceneAudioSystem>(services)
              : nullptr),
          config(initialConfig) {}

    Scene* activeScene = nullptr;
    core::EngineServices* services = nullptr;
    script::NativeScriptSystem nativeScripts;
    physics::PhysicsSystem2D physics2D;
    physics::PhysicsSystem3D physics3D;
    std::unique_ptr<SceneRenderer> renderer;
    std::unique_ptr<SceneAudioSystem> audio;
    SceneRuntimeConfig config;
    std::array<std::vector<SceneSystemCallback>, 6> systems;
    bool paused = false;

    void invoke(const SceneRuntimePhase phase, const float deltaTime) {
        for (auto& system : systems[phaseIndex(phase)]) {
            system(*activeScene, deltaTime);
        }
    }
};

SceneRuntime::SceneRuntime(script::NativeScriptRegistry& scriptRegistry)
    : m_impl(std::make_unique<Impl>(scriptRegistry)) {}

SceneRuntime::SceneRuntime(
    core::EngineServices& services,
    const SceneRuntimeConfig config
) : m_impl(std::make_unique<Impl>(services, config)) {}

SceneRuntime::~SceneRuntime() {
    stop();
}

SceneRuntime::SceneRuntime(SceneRuntime&&) noexcept = default;

SceneRuntime& SceneRuntime::operator=(SceneRuntime&& other) noexcept {
    if (this != &other) {
        stop();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

void SceneRuntime::play(Scene& sceneToPlay) {
    if (!m_impl) {
        throw std::logic_error("Cannot use a moved-from SceneRuntime");
    }
    if (isPlaying()) {
        throw std::logic_error("SceneRuntime is already playing a scene");
    }

    m_impl->activeScene = &sceneToPlay;
    m_impl->paused = false;
    try {
        if (m_impl->config.physics2D) {
            m_impl->physics2D.rebuild(sceneToPlay);
        }
        if (m_impl->config.physics3D) {
            m_impl->physics3D.rebuild(sceneToPlay);
        }
        if (m_impl->audio) {
            m_impl->audio->attachScene(sceneToPlay);
        }
        if (m_impl->services) {
            m_impl->nativeScripts.attachScene(sceneToPlay, *m_impl->services, *this);
        } else {
            m_impl->nativeScripts.attachScene(sceneToPlay);
        }
    } catch (...) {
        m_impl->nativeScripts.detachScene();
        if (m_impl->audio) {
            m_impl->audio->detachScene();
        }
        clearPhysics(m_impl->physics2D, m_impl->physics3D);
        m_impl->activeScene = nullptr;
        throw;
    }
}

void SceneRuntime::update(const float deltaTime) {
    if (!isPlaying()) {
        throw std::logic_error("SceneRuntime requires a playing scene");
    }
    m_impl->invoke(SceneRuntimePhase::BeforeUpdate, deltaTime);
    m_impl->nativeScripts.update(deltaTime);
    if (m_impl->audio) {
        m_impl->audio->update();
    }
    m_impl->invoke(SceneRuntimePhase::AfterUpdate, deltaTime);
}

void SceneRuntime::fixedUpdate(const float fixedDeltaTime) {
    if (!isPlaying()) {
        throw std::logic_error("SceneRuntime requires a playing scene");
    }
    m_impl->invoke(SceneRuntimePhase::BeforePhysics, fixedDeltaTime);
    m_impl->nativeScripts.fixedUpdate(fixedDeltaTime);
    if (m_impl->config.physics2D) {
        m_impl->physics2D.update(*m_impl->activeScene, fixedDeltaTime);
    }
    if (m_impl->config.physics3D) {
        m_impl->physics3D.update(*m_impl->activeScene, fixedDeltaTime);
    }
    m_impl->invoke(SceneRuntimePhase::AfterPhysics, fixedDeltaTime);
}

bool SceneRuntime::render(
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight
) {
    if (!isPlaying()) {
        throw std::logic_error("SceneRuntime requires a playing scene");
    }
    m_impl->invoke(SceneRuntimePhase::BeforeRender, 0.0F);
    const bool rendered = m_impl->renderer &&
        m_impl->renderer->render(*m_impl->activeScene, viewportWidth, viewportHeight);
    m_impl->invoke(SceneRuntimePhase::AfterRender, 0.0F);
    return rendered;
}

void SceneRuntime::addSystem(
    const SceneRuntimePhase phase,
    SceneSystemCallback callback
) {
    if (!callback) {
        throw std::invalid_argument("A scene system callback cannot be empty");
    }
    m_impl->systems[phaseIndex(phase)].push_back(std::move(callback));
}

void SceneRuntime::stop() noexcept {
    if (!m_impl || !isPlaying()) {
        return;
    }

    m_impl->nativeScripts.detachScene();
    if (m_impl->audio) {
        m_impl->audio->detachScene();
    }
    clearPhysics(m_impl->physics2D, m_impl->physics3D);
    m_impl->activeScene = nullptr;
    m_impl->paused = false;
}

void SceneRuntime::setPaused(const bool paused) noexcept {
    if (m_impl && isPlaying()) {
        m_impl->paused = paused;
    }
}

bool SceneRuntime::isPaused() const noexcept {
    return m_impl && m_impl->paused;
}

bool SceneRuntime::isPlaying() const noexcept {
    return m_impl && m_impl->activeScene != nullptr;
}

Scene* SceneRuntime::scene() noexcept {
    return m_impl ? m_impl->activeScene : nullptr;
}

const Scene* SceneRuntime::scene() const noexcept {
    return m_impl ? m_impl->activeScene : nullptr;
}

script::NativeScriptSystem& SceneRuntime::nativeScripts() noexcept {
    return m_impl->nativeScripts;
}

const script::NativeScriptSystem& SceneRuntime::nativeScripts() const noexcept {
    return m_impl->nativeScripts;
}

physics::PhysicsSystem2D& SceneRuntime::physics2D() noexcept {
    return m_impl->physics2D;
}

const physics::PhysicsSystem2D& SceneRuntime::physics2D() const noexcept {
    return m_impl->physics2D;
}

physics::PhysicsSystem3D& SceneRuntime::physics3D() noexcept {
    return m_impl->physics3D;
}

const physics::PhysicsSystem3D& SceneRuntime::physics3D() const noexcept {
    return m_impl->physics3D;
}

SceneRenderer* SceneRuntime::sceneRenderer() noexcept {
    return m_impl ? m_impl->renderer.get() : nullptr;
}

SceneAudioSystem* SceneRuntime::sceneAudio() noexcept {
    return m_impl ? m_impl->audio.get() : nullptr;
}

} // namespace vshade::scene
