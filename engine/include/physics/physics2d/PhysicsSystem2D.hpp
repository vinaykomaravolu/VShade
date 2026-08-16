#pragma once

#include "physics/physics2d/PhysicsWorld2D.hpp"
#include "scene/Entity.hpp"

#include <memory>
#include <optional>

namespace vshade::scene {
class Scene;
}

namespace vshade::physics {

/** @brief A ray hit mapped back to the scene entity that owns the body. */
struct SceneRaycastHit2D {
    scene::Entity entity;
    RaycastHit2D physics;
};

struct SceneContactEvent2D {
    scene::Entity first;
    scene::Entity second;
    math::Vec2 normal{0.0F};
    math::Vec2 point{0.0F};
    ContactPhase phase = ContactPhase::Began;
};
using SceneContactListener2D = std::function<void(const SceneContactEvent2D&)>;

/** @brief Synchronizes scene components with their runtime Box2D bodies. */
class PhysicsSystem2D final {
public:
    explicit PhysicsSystem2D(const PhysicsWorld2DSettings& settings = {});
    ~PhysicsSystem2D();

    PhysicsSystem2D(PhysicsSystem2D&&) noexcept;
    PhysicsSystem2D& operator=(PhysicsSystem2D&&) noexcept;
    PhysicsSystem2D(const PhysicsSystem2D&) = delete;
    PhysicsSystem2D& operator=(const PhysicsSystem2D&) = delete;

    /** @brief Recreates every runtime body from the scene's current components. */
    void rebuild(scene::Scene& scene);

    /**
     * @brief Synchronizes transforms and advances Box2D by one fixed time step.
     *
     * Static and kinematic transforms flow from the scene into Box2D. Dynamic
     * transforms flow from Box2D back into the scene after simulation.
     */
    void update(scene::Scene& scene, float fixedDeltaTime);
    void clear();

    [[nodiscard]] std::optional<PhysicsBody2D> body(scene::Entity entity) const noexcept;
    void applyImpulse(scene::Entity entity, const math::Vec2& impulse);
    [[nodiscard]] std::optional<SceneRaycastHit2D> raycast(
        const RaycastQuery2D& query
    ) const;
    void setContactListener(SceneContactListener2D listener);

    [[nodiscard]] PhysicsWorld2D& world() noexcept;
    [[nodiscard]] const PhysicsWorld2D& world() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
