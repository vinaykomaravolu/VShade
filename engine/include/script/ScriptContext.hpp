#pragma once

#include "scene/Entity.hpp"

namespace vshade::asset { class AssetManager; }
namespace vshade::audio { class AudioService; }
namespace vshade::core { class EngineServices; }
namespace vshade::physics { class PhysicsSystem2D; class PhysicsSystem3D; }
namespace vshade::scene { class Scene; class SceneRuntime; }

namespace vshade::script {

/** @brief Typed access to the world and application services available to a script. */
class ScriptContext final {
public:
    ScriptContext() = default;

    [[nodiscard]] scene::Entity entity() const noexcept;
    [[nodiscard]] scene::Scene& scene() const;
    [[nodiscard]] bool hasServices() const noexcept;
    [[nodiscard]] core::EngineServices& services() const;
    [[nodiscard]] asset::AssetManager& assets() const;
    [[nodiscard]] audio::AudioService& audio() const;
    [[nodiscard]] scene::SceneRuntime& runtime() const;
    [[nodiscard]] physics::PhysicsSystem2D& physics2D() const;
    [[nodiscard]] physics::PhysicsSystem3D& physics3D() const;

private:
    friend class NativeScriptSystem;

    ScriptContext(
        scene::Entity owner,
        scene::Scene& ownerScene,
        core::EngineServices* engineServices,
        scene::SceneRuntime* sceneRuntime
    ) noexcept;

    scene::Entity m_entity;
    scene::Scene* m_scene = nullptr;
    core::EngineServices* m_services = nullptr;
    scene::SceneRuntime* m_runtime = nullptr;
};

} // namespace vshade::script
