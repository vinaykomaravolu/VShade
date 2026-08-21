#pragma once

#include "math/Vector.hpp"

#include <variant>
#include <cstdint>
#include <vector>

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

/** @brief Y-axis cylinder collider centered on its body. */
struct CylinderShape3D {
    float halfHeight = 0.5F;
    float radius = 0.5F;
};

/** @brief Triangle mesh collider geometry, normally generated from a Model. */
struct MeshShape3D {
    std::vector<math::Vec3> vertices;
    std::vector<std::uint32_t> indices;
};

/** @brief Point cloud used to construct a convex hull collider. */
struct ConvexShape3D {
    std::vector<math::Vec3> points;
};

/** @brief Supported Jolt collider descriptions. */
using PhysicsShape3D = std::variant<
    BoxShape3D,
    SphereShape3D,
    CapsuleShape3D,
    CylinderShape3D,
    MeshShape3D,
    ConvexShape3D
>;

} // namespace vshade::physics
