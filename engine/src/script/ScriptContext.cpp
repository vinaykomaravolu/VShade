#include "script/ScriptContext.hpp"

#include "core/EngineServices.hpp"
#include "scene/SceneRuntime.hpp"

#include <stdexcept>

namespace vshade::script {

ScriptContext::ScriptContext(
    const scene::Entity owner,
    scene::Scene& ownerScene,
    core::EngineServices* const engineServices,
    scene::SceneRuntime* const sceneRuntime
) noexcept
    : m_entity(owner),
      m_scene(&ownerScene),
      m_services(engineServices),
      m_runtime(sceneRuntime) {}

scene::Entity ScriptContext::entity() const noexcept { return m_entity; }

scene::Scene& ScriptContext::scene() const {
    if (m_scene == nullptr) throw std::logic_error("Script context has no scene");
    return *m_scene;
}

bool ScriptContext::hasServices() const noexcept {
    return m_services != nullptr && m_runtime != nullptr;
}

core::EngineServices& ScriptContext::services() const {
    if (m_services == nullptr) {
        throw std::logic_error("Script context has no engine services");
    }
    return *m_services;
}

asset::AssetManager& ScriptContext::assets() const { return services().assets(); }
audio::AudioService& ScriptContext::audio() const { return services().audio(); }

scene::SceneRuntime& ScriptContext::runtime() const {
    if (m_runtime == nullptr) {
        throw std::logic_error("Script context has no scene runtime");
    }
    return *m_runtime;
}

physics::PhysicsSystem2D& ScriptContext::physics2D() const {
    return runtime().physics2D();
}

physics::PhysicsSystem3D& ScriptContext::physics3D() const {
    return runtime().physics3D();
}

} // namespace vshade::script
