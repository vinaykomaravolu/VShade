#pragma once

#include "math/Quaternion.hpp"
#include "math/Vector.hpp"
#include "physics/physics3d/Collision3D.hpp"
#include "physics/physics3d/PhysicsBody3D.hpp"
#include "physics/physics3d/PhysicsMaterial3D.hpp"
#include "physics/physics3d/PhysicsShape3D.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace vshade::physics {

/** @brief Configuration used to initialize a Jolt simulation. */
struct PhysicsWorld3DSettings {
    math::Vec3 gravity{0.0F, -9.81F, 0.0F};
    std::uint32_t maxBodies = 65'536;
};

/** @brief Owns one isolated Jolt simulation and all bodies created in it. */
class PhysicsWorld3D final {
public:
    explicit PhysicsWorld3D(const PhysicsWorld3DSettings& settings = {});
    ~PhysicsWorld3D();

    PhysicsWorld3D(PhysicsWorld3D&&) noexcept;
    PhysicsWorld3D& operator=(PhysicsWorld3D&&) noexcept;
    PhysicsWorld3D(const PhysicsWorld3D&) = delete;
    PhysicsWorld3D& operator=(const PhysicsWorld3D&) = delete;

    [[nodiscard]] PhysicsBody3D createBody(
        const PhysicsBody3DSettings& settings,
        const PhysicsShape3D& shape,
        const PhysicsMaterial3D& material = {},
        const math::Vec3& colliderOffset = math::Vec3{0.0F},
        bool sensor = false
    );
    void destroyBody(PhysicsBody3D body);
    [[nodiscard]] bool contains(PhysicsBody3D body) const noexcept;

    void step(float fixedDeltaTime);
    void setGravity(const math::Vec3& gravity);
    [[nodiscard]] math::Vec3 gravity() const;

    void setTransform(
        PhysicsBody3D body,
        const math::Vec3& position,
        const math::Quat& rotation
    );
    [[nodiscard]] math::Vec3 position(PhysicsBody3D body) const;
    [[nodiscard]] math::Quat rotation(PhysicsBody3D body) const;
    void setLinearVelocity(PhysicsBody3D body, const math::Vec3& velocity);
    [[nodiscard]] math::Vec3 linearVelocity(PhysicsBody3D body) const;
    void applyForce(PhysicsBody3D body, const math::Vec3& force);
    void applyImpulse(PhysicsBody3D body, const math::Vec3& impulse);

    [[nodiscard]] std::optional<RaycastHit3D> raycast(const RaycastQuery3D& query) const;
    void setContactListener(ContactListener3D listener);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
