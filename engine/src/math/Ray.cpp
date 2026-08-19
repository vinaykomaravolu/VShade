#include "math/Ray.hpp"

#include <cmath>

namespace vshade::math {

std::optional<Vec3> intersectPlane(
    const Ray& ray,
    const Vec3& planePoint,
    const Vec3& planeNormal,
    const float epsilon
) {
    const Vec3 direction = normalizedOrZero(ray.direction, epsilon);
    const Vec3 normal = normalizedOrZero(planeNormal, epsilon);
    if (lengthSquared(direction) <= 0.0F || lengthSquared(normal) <= 0.0F) {
        return std::nullopt;
    }

    const float denominator = dot(direction, normal);
    if (std::abs(denominator) <= epsilon) {
        return std::nullopt;
    }

    const float distance = dot(planePoint - ray.origin, normal) / denominator;
    if (distance < 0.0F) {
        return std::nullopt;
    }
    return ray.origin + direction * distance;
}

} // namespace vshade::math
