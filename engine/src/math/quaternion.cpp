#include "math/quaternion.hpp"

#include <limits>
#include <stdexcept>

#include <glm/common.hpp>

namespace vshade::math {

Quat identity() noexcept {
    return {1.0F, 0.0F, 0.0F, 0.0F};
}

Quat fromEuler(const Vec3& eulerRadians) {
    return normalize(Quat{eulerRadians});
}

Vec3 toEuler(const Quat& quaternion) {
    return glm::eulerAngles(normalize(quaternion));
}

Quat fromAxisAngle(const Vec3& axis, const float angleRadians) {
    if (glm::dot(axis, axis) <= std::numeric_limits<float>::epsilon()) {
        throw std::invalid_argument("Quaternion rotation axis must not be zero");
    }

    return glm::angleAxis(angleRadians, glm::normalize(axis));
}

Quat normalize(const Quat& quaternion) {
    if (glm::dot(quaternion, quaternion) <= std::numeric_limits<float>::epsilon()) {
        throw std::invalid_argument("Quaternion must not have zero length");
    }

    return glm::normalize(quaternion);
}

Quat inverse(const Quat& quaternion) {
    return glm::inverse(normalize(quaternion));
}

Quat slerp(const Quat& start, const Quat& end, const float t) {
    const float clampedT = glm::clamp(t, 0.0F, 1.0F);
    return normalize(glm::slerp(
        normalize(start),
        normalize(end),
        clampedT
    ));
}

Vec3 rotate(const Quat& quaternion, const Vec3& vector) {
    return normalize(quaternion) * vector;
}

Vec3 forward(const Quat& rotation) {
    return rotate(rotation, {0.0F, 0.0F, -1.0F});
}

Vec3 right(const Quat& rotation) {
    return rotate(rotation, {1.0F, 0.0F, 0.0F});
}

Vec3 up(const Quat& rotation) {
    return rotate(rotation, {0.0F, 1.0F, 0.0F});
}

} // namespace vshade::math
