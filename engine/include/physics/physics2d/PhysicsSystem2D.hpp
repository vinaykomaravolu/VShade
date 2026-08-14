#pragma once

#include "physics/physics2d/PhysicsWorld2D.hpp"

#include <memory>

namespace vshade::scene {
class Scene;
}

namespace vshade::physics {

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

    [[nodiscard]] PhysicsWorld2D& world() noexcept;
    [[nodiscard]] const PhysicsWorld2D& world() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
