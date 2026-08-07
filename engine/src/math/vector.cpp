#include "math/vector.hpp"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>

namespace vshade::math {
namespace {

template<typename Vector>
[[nodiscard]] float lengthSquaredImpl(const Vector& vector) noexcept {
    return glm::dot(vector, vector);
}

template<typename Vector>
[[nodiscard]] Vector normalizedOrZeroImpl(const Vector& vector, const float epsilon) noexcept {
    const float squared_length = lengthSquaredImpl(vector);
    const float non_negative_epsilon = std::max(epsilon, 0.0F);
    if (squared_length <= non_negative_epsilon * non_negative_epsilon) {
        return Vector{0.0F};
    }

    return vector / std::sqrt(squared_length);
}

template<typename Vector>
[[nodiscard]] Vector moveTowardsImpl(
    const Vector& current,
    const Vector& target,
    const float maxDistanceDelta
) noexcept {
    if (maxDistanceDelta <= 0.0F) {
        return current;
    }

    const Vector offset = target - current;
    const float squared_distance = lengthSquaredImpl(offset);
    if (squared_distance <= maxDistanceDelta * maxDistanceDelta) {
        return target;
    }

    return current + offset * (maxDistanceDelta / std::sqrt(squared_distance));
}

template<typename Vector>
[[nodiscard]] float angleBetweenImpl(const Vector& first, const Vector& second) noexcept {
    const float squared_lengths = lengthSquaredImpl(first) * lengthSquaredImpl(second);
    if (squared_lengths <= 0.0F) {
        return 0.0F;
    }

    const float cosine = glm::dot(first, second) / std::sqrt(squared_lengths);
    return std::acos(std::clamp(cosine, -1.0F, 1.0F));
}

} // namespace

float length(const Vec2& v) noexcept {
    return glm::length(v);
}

float length(const Vec3& v) noexcept {
    return glm::length(v);
}

float length(const Vec4& v) noexcept {
    return glm::length(v);
}

Vec2 normalize(const Vec2& v) noexcept {
    return glm::normalize(v);
}

Vec3 normalize(const Vec3& v) noexcept {
    return glm::normalize(v);
}

Vec4 normalize(const Vec4& v) noexcept {
    return glm::normalize(v);
}

float dot(const Vec2& a, const Vec2& b) noexcept {
    return glm::dot(a, b);
}

float dot(const Vec3& a, const Vec3& b) noexcept {
    return glm::dot(a, b);
}

float dot(const Vec4& a, const Vec4& b) noexcept {
    return glm::dot(a, b);
}

Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
    return glm::cross(a, b);
}

float distance(const Vec2& a, const Vec2& b) noexcept {
    return glm::distance(a, b);
}

float distance(const Vec3& a, const Vec3& b) noexcept {
    return glm::distance(a, b);
}

Vec3 reflect(const Vec3& direction, const Vec3& normal) noexcept {
    return glm::reflect(direction, normal);
}

Vec2 lerp(const Vec2& a, const Vec2& b, const float t) noexcept {
    return a + (b - a) * t;
}

Vec3 lerp(const Vec3& a, const Vec3& b, const float t) noexcept {
    return a + (b - a) * t;
}

float lengthSquared(const Vec2& vector) noexcept {
    return lengthSquaredImpl(vector);
}

float lengthSquared(const Vec3& vector) noexcept {
    return lengthSquaredImpl(vector);
}

float distanceSquared(const Vec2& first, const Vec2& second) noexcept {
    return lengthSquared(second - first);
}

float distanceSquared(const Vec3& first, const Vec3& second) noexcept {
    return lengthSquared(second - first);
}

Vec2 normalizedOrZero(const Vec2& vector, const float epsilon) noexcept {
    return normalizedOrZeroImpl(vector, epsilon);
}

Vec3 normalizedOrZero(const Vec3& vector, const float epsilon) noexcept {
    return normalizedOrZeroImpl(vector, epsilon);
}

Vec2 direction(const Vec2& from, const Vec2& to) noexcept {
    return normalizedOrZero(to - from);
}

Vec3 direction(const Vec3& from, const Vec3& to) noexcept {
    return normalizedOrZero(to - from);
}

Vec2 moveTowards(
    const Vec2& current,
    const Vec2& target,
    const float maxDistanceDelta
) noexcept {
    return moveTowardsImpl(current, target, maxDistanceDelta);
}

Vec3 moveTowards(
    const Vec3& current,
    const Vec3& target,
    const float maxDistanceDelta
) noexcept {
    return moveTowardsImpl(current, target, maxDistanceDelta);
}

float angleBetween(const Vec2& first, const Vec2& second) noexcept {
    return angleBetweenImpl(first, second);
}

float angleBetween(const Vec3& first, const Vec3& second) noexcept {
    return angleBetweenImpl(first, second);
}

} // namespace vshade::math
