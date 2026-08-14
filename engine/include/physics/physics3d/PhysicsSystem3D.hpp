#pragma once

#include "physics/physics3d/PhysicsWorld3D.hpp"

#include <memory>

namespace vshade::scene {
class Scene;
}

namespace vshade::physics {

/** @brief Synchronizes scene components with their runtime Jolt bodies. */
class PhysicsSystem3D final {
public:
    explicit PhysicsSystem3D(const PhysicsWorld3DSettings& settings = {});
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

    [[nodiscard]] PhysicsWorld3D& world() noexcept;
    [[nodiscard]] const PhysicsWorld3D& world() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
