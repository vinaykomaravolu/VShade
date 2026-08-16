#pragma once

#include "math/Vector.hpp"

#include <variant>

namespace vshade::physics {

/** @brief Box collider dimensions before body rotation is applied. */
struct BoxShape3D {
    math::Vec3 halfExtents{0.5F};
};

/** @brief Sphere collider centered on its body. */
struct SphereShape3D {
    float radius = 0.5F;
};

/** @brief Y-axis capsule collider centered on its body. */
struct CapsuleShape3D {
    float halfHeight = 0.5F;
    float radius = 0.25F;
};

/** @brief Supported Jolt collider descriptions. */
using PhysicsShape3D = std::variant<BoxShape3D, SphereShape3D, CapsuleShape3D>;

} // namespace vshade::physics
