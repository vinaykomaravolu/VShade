#pragma once

#include "math/Vector.hpp"

#include <optional>

namespace vshade::math {

/** @brief Infinite directed line used for picking and spawn placement. */
struct Ray {
    Vec3 origin{0.0F};
    Vec3 direction{0.0F, 0.0F, -1.0F};
};

/**
 * @brief Returns the first forward intersection of @p ray with a plane.
 * @return No value when the ray is degenerate, parallel, or hits behind the origin.
 */
[[nodiscard]] std::optional<Vec3> intersectPlane(
    const Ray& ray,
    const Vec3& planePoint,
    const Vec3& planeNormal,
    float epsilon = 0.000001F
);

} // namespace vshade::math
