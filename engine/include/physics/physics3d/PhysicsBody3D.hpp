#pragma once

#include "math/Quaternion.hpp"
#include "math/Vector.hpp"
#include "physics/PhysicsTypes.hpp"

#include <cstdint>

namespace vshade::physics {

/** @brief Initial state used when creating a body in a three-dimensional world. */
struct PhysicsBody3DSettings {
    BodyType type = BodyType::Static;
    math::Vec3 position{0.0F};
    math::Quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
    math::Vec3 linearVelocity{0.0F};
    math::Vec3 angularVelocity{0.0F};
    float mass = 1.0F;
    float linearDamping = 0.05F;
    float angularDamping = 0.05F;
    float gravityScale = 1.0F;
    bool continuousCollision = false;
    bool enabled = true;
    CollisionFilter collision;
    std::uint64_t userData = 0;
};

/**
 * @brief Lightweight reference to a body owned by PhysicsWorld3D.
 *
 * The handle contains no Jolt types and does not own the referenced body.
 */
class PhysicsBody3D final {
public:
    PhysicsBody3D() = default;

    [[nodiscard]] BodyId3D id() const noexcept { return m_id; }
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(m_id); }

    bool operator==(const PhysicsBody3D&) const = default;

private:
    explicit PhysicsBody3D(BodyId3D id) : m_id(id) {}

    BodyId3D m_id;

    friend class PhysicsWorld3D;
};

} // namespace vshade::physics
