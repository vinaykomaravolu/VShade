#pragma once

#include "math/Vector.hpp"
#include "physics/PhysicsTypes.hpp"

#include <cstdint>

namespace vshade::physics {

/** @brief Initial state used when creating a body in a two-dimensional world. */
struct PhysicsBody2DSettings {
    BodyType type = BodyType::Static;
    math::Vec2 position{0.0F};
    float rotationRadians = 0.0F;
    math::Vec2 linearVelocity{0.0F};
    float angularVelocity = 0.0F;
    float linearDamping = 0.0F;
    float angularDamping = 0.0F;
    float gravityScale = 1.0F;
    bool fixedRotation = false;
    bool continuousCollision = false;
    bool enabled = true;
    CollisionFilter collision;
    std::uint64_t userData = 0;
};

/**
 * @brief Lightweight reference to a body owned by PhysicsWorld2D.
 *
 * The handle contains no Box2D types and does not own the referenced body.
 */
class PhysicsBody2D final {
public:
    PhysicsBody2D() = default;

    [[nodiscard]] BodyId2D id() const noexcept { return m_id; }
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(m_id); }

    bool operator==(const PhysicsBody2D&) const = default;

private:
    explicit PhysicsBody2D(BodyId2D id) : m_id(id) {}

    BodyId2D m_id;

    friend class PhysicsWorld2D;
};

} // namespace vshade::physics
