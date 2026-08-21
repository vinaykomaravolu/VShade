#pragma once

#include "physics/physics3d/PhysicsWorld3D.hpp"
#include "scene/Entity.hpp"

#include <memory>
#include <optional>

namespace vshade::scene {
class Scene;
}

namespace vshade::asset { class AssetManager; }

namespace vshade::physics {

/** @brief A ray hit mapped back to the scene entity that owns the body. */
struct SceneRaycastHit3D {
    scene::Entity entity;
    RaycastHit3D physics;
};

struct SceneContactEvent3D {
    scene::Entity first;
    scene::Entity second;
    math::Vec3 normal{0.0F};
    math::Vec3 point{0.0F};
    ContactPhase phase = ContactPhase::Began;
};
using SceneContactListener3D = std::function<void(const SceneContactEvent3D&)>;

/** @brief Synchronizes scene components with their runtime Jolt bodies. */
class PhysicsSystem3D final {
public:
    explicit PhysicsSystem3D(
        const PhysicsWorld3DSettings& settings = {},
        asset::AssetManager* assets = nullptr
    );
    ~PhysicsSystem3D();

    PhysicsSystem3D(PhysicsSystem3D&&) noexcept;
    PhysicsSystem3D& operator=(PhysicsSystem3D&&) noexcept;
    PhysicsSystem3D(const PhysicsSystem3D&) = delete;
    PhysicsSystem3D& operator=(const PhysicsSystem3D&) = delete;

    /** @brief Recreates every runtime body from the scene's current components. */
    void rebuild(scene::Scene& scene);

    /**
     * @brief Synchronizes transforms and advances Jolt by one fixed time step.
     *
     * Static and kinematic transforms flow from the scene into Jolt. Dynamic
     * transforms flow from Jolt back into the scene after simulation.
     */
    void update(scene::Scene& scene, float fixedDeltaTime);
    void clear();

    /** @brief Finds the runtime body owned by an entity. */
    [[nodiscard]] std::optional<PhysicsBody3D> body(scene::Entity entity) const noexcept;

    /** @brief Applies an impulse without exposing the body lookup table. */
    void applyImpulse(scene::Entity entity, const math::Vec3& impulse);

    /** @brief Casts a ray and maps the hit body back to an entity. */
    [[nodiscard]] std::optional<SceneRaycastHit3D> raycast(
        const RaycastQuery3D& query
    ) const;
    void setContactListener(SceneContactListener3D listener);

    [[nodiscard]] PhysicsWorld3D& world() noexcept;
    [[nodiscard]] const PhysicsWorld3D& world() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
