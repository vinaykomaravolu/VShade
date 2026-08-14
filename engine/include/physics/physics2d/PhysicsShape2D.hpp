#pragma once

#include "math/Vector.hpp"

#include <variant>

namespace vshade::physics {

/** @brief Axis-aligned box dimensions before body rotation is applied. */
struct BoxShape2D {
    math::Vec2 halfExtents{0.5F};
};

/** @brief Circle collider centered on its body. */
struct CircleShape2D {
    float radius = 0.5F;
};

/** @brief Vertical capsule collider centered on its body. */
struct CapsuleShape2D {
    float halfHeight = 0.5F;
    float radius = 0.25F;
};

/** @brief Supported Box2D collider descriptions. */
using PhysicsShape2D = std::variant<BoxShape2D, CircleShape2D, CapsuleShape2D>;

} // namespace vshade::physics
