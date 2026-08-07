#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace vshade::math {

/** @brief Two-component floating-point vector. */
using Vec2 = glm::vec2;

/** @brief Three-component floating-point vector. */
using Vec3 = glm::vec3;

/** @brief Four-component floating-point vector. */
using Vec4 = glm::vec4;

/** @brief Two-component double-precision vector. */
using DVec2 = glm::dvec2;

/** @brief Three-component double-precision vector. */
using DVec3 = glm::dvec3;

/** @brief Four-component double-precision vector. */
using DVec4 = glm::dvec4;

/** @brief Two-component signed integer vector. */
using IVec2 = glm::ivec2;

/** @brief Three-component signed integer vector. */
using IVec3 = glm::ivec3;

/** @brief Four-component signed integer vector. */
using IVec4 = glm::ivec4;

/** @brief Two-component unsigned integer vector. */
using UVec2 = glm::uvec2;

/** @brief Three-component unsigned integer vector. */
using UVec3 = glm::uvec3;

/** @brief Four-component unsigned integer vector. */
using UVec4 = glm::uvec4;

/** @brief Returns the Euclidean length of a two-component vector. */
[[nodiscard]] float length(const Vec2& v) noexcept;

/** @brief Returns the Euclidean length of a three-component vector. */
[[nodiscard]] float length(const Vec3& v) noexcept;

/** @brief Returns the Euclidean length of a four-component vector. */
[[nodiscard]] float length(const Vec4& v) noexcept;

/** @brief Returns a unit-length copy of a non-zero vector. */
[[nodiscard]] Vec2 normalize(const Vec2& v) noexcept;

/** @brief Returns a unit-length copy of a non-zero vector. */
[[nodiscard]] Vec3 normalize(const Vec3& v) noexcept;

/** @brief Returns a unit-length copy of a non-zero vector. */
[[nodiscard]] Vec4 normalize(const Vec4& v) noexcept;

/** @brief Returns the dot product of two vectors. */
[[nodiscard]] float dot(const Vec2& a, const Vec2& b) noexcept;

/** @brief Returns the dot product of two vectors. */
[[nodiscard]] float dot(const Vec3& a, const Vec3& b) noexcept;

/** @brief Returns the dot product of two vectors. */
[[nodiscard]] float dot(const Vec4& a, const Vec4& b) noexcept;

/** @brief Returns a vector perpendicular to both @p a and @p b. */
[[nodiscard]] Vec3 cross(const Vec3& a, const Vec3& b) noexcept;

/** @brief Returns the distance between two points. */
[[nodiscard]] float distance(const Vec2& a, const Vec2& b) noexcept;

/** @brief Returns the distance between two points. */
[[nodiscard]] float distance(const Vec3& a, const Vec3& b) noexcept;

/**
 * @brief Reflects @p direction from a surface.
 * @param normal Unit-length surface normal.
 */
[[nodiscard]] Vec3 reflect(const Vec3& direction, const Vec3& normal) noexcept;

/**
 * @brief Linearly interpolates from @p a to @p b.
 * @param t Blend amount; values outside zero through one extrapolate.
 */
[[nodiscard]] Vec2 lerp(const Vec2& a, const Vec2& b, float t) noexcept;

/**
 * @brief Linearly interpolates from @p a to @p b.
 * @param t Blend amount; values outside zero through one extrapolate.
 */
[[nodiscard]] Vec3 lerp(const Vec3& a, const Vec3& b, float t) noexcept;

/** @brief Returns the squared length without performing a square root. */
[[nodiscard]] float lengthSquared(const Vec2& vector) noexcept;

/** @brief Returns the squared length without performing a square root. */
[[nodiscard]] float lengthSquared(const Vec3& vector) noexcept;

/** @brief Returns the squared distance between two points. */
[[nodiscard]] float distanceSquared(const Vec2& first, const Vec2& second) noexcept;

/** @brief Returns the squared distance between two points. */
[[nodiscard]] float distanceSquared(const Vec3& first, const Vec3& second) noexcept;

/**
 * @brief Returns a unit-length copy, or zero when the vector is too small.
 * @param epsilon Length at or below which the vector is treated as zero.
 */
[[nodiscard]] Vec2 normalizedOrZero(const Vec2& vector, float epsilon = 0.000001F) noexcept;

/**
 * @brief Returns a unit-length copy, or zero when the vector is too small.
 * @param epsilon Length at or below which the vector is treated as zero.
 */
[[nodiscard]] Vec3 normalizedOrZero(const Vec3& vector, float epsilon = 0.000001F) noexcept;

/** @brief Returns the unit direction from @p from to @p to, or zero if they coincide. */
[[nodiscard]] Vec2 direction(const Vec2& from, const Vec2& to) noexcept;

/** @brief Returns the unit direction from @p from to @p to, or zero if they coincide. */
[[nodiscard]] Vec3 direction(const Vec3& from, const Vec3& to) noexcept;

/**
 * @brief Moves toward @p target without travelling farther than @p maxDistanceDelta.
 * @param maxDistanceDelta Non-negative maximum distance to move this call.
 */
[[nodiscard]] Vec2 moveTowards(
    const Vec2& current,
    const Vec2& target,
    float maxDistanceDelta
) noexcept;

/**
 * @brief Moves toward @p target without travelling farther than @p maxDistanceDelta.
 * @param maxDistanceDelta Non-negative maximum distance to move this call.
 */
[[nodiscard]] Vec3 moveTowards(
    const Vec3& current,
    const Vec3& target,
    float maxDistanceDelta
) noexcept;

/** @brief Returns the unsigned angle between two vectors in radians. */
[[nodiscard]] float angleBetween(const Vec2& first, const Vec2& second) noexcept;

/** @brief Returns the unsigned angle between two vectors in radians. */
[[nodiscard]] float angleBetween(const Vec3& first, const Vec3& second) noexcept;

} // namespace vshade::math
