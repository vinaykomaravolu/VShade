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

/**
 * @brief Returns the Euclidean length of a two-component vector.
 * @param v Vector to measure.
 * @return Euclidean length of @p v.
 */
[[nodiscard]] float length(const Vec2& v) noexcept;

/**
 * @brief Returns the Euclidean length of a three-component vector.
 * @param v Vector to measure.
 * @return Euclidean length of @p v.
 */
[[nodiscard]] float length(const Vec3& v) noexcept;

/**
 * @brief Returns the Euclidean length of a four-component vector.
 * @param v Vector to measure.
 * @return Euclidean length of @p v.
 */
[[nodiscard]] float length(const Vec4& v) noexcept;

/**
 * @brief Returns a unit-length copy of a non-zero vector.
 * @param v Vector to normalize.
 * @return Unit-length vector pointing in the same direction as @p v.
 * @warning Use normalizedOrZero() when @p v may have zero length.
 */
[[nodiscard]] Vec2 normalize(const Vec2& v) noexcept;

/**
 * @brief Returns a unit-length copy of a non-zero vector.
 * @param v Vector to normalize.
 * @return Unit-length vector pointing in the same direction as @p v.
 * @warning Use normalizedOrZero() when @p v may have zero length.
 */
[[nodiscard]] Vec3 normalize(const Vec3& v) noexcept;

/**
 * @brief Returns a unit-length copy of a non-zero vector.
 * @param v Vector to normalize.
 * @return Unit-length vector pointing in the same direction as @p v.
 * @warning Passing a zero-length vector produces undefined components.
 */
[[nodiscard]] Vec4 normalize(const Vec4& v) noexcept;

/**
 * @brief Returns the dot product of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Scalar dot product of @p a and @p b.
 */
[[nodiscard]] float dot(const Vec2& a, const Vec2& b) noexcept;

/**
 * @brief Returns the dot product of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Scalar dot product of @p a and @p b.
 */
[[nodiscard]] float dot(const Vec3& a, const Vec3& b) noexcept;

/**
 * @brief Returns the dot product of two vectors.
 * @param a First vector.
 * @param b Second vector.
 * @return Scalar dot product of @p a and @p b.
 */
[[nodiscard]] float dot(const Vec4& a, const Vec4& b) noexcept;

/**
 * @brief Returns a vector perpendicular to both @p a and @p b.
 * @param a First vector.
 * @param b Second vector.
 * @return Vector perpendicular to @p a and @p b.
 */
[[nodiscard]] Vec3 cross(const Vec3& a, const Vec3& b) noexcept;

/**
 * @brief Returns the distance between two points.
 * @param a First point.
 * @param b Second point.
 * @return Euclidean distance between @p a and @p b.
 */
[[nodiscard]] float distance(const Vec2& a, const Vec2& b) noexcept;

/**
 * @brief Returns the distance between two points.
 * @param a First point.
 * @param b Second point.
 * @return Euclidean distance between @p a and @p b.
 */
[[nodiscard]] float distance(const Vec3& a, const Vec3& b) noexcept;

/**
 * @brief Reflects @p direction from a surface.
 * @param direction Incoming direction to reflect.
 * @param normal Unit-length surface normal.
 * @return Direction reflected around @p normal.
 */
[[nodiscard]] Vec3 reflect(const Vec3& direction, const Vec3& normal) noexcept;

/**
 * @brief Linearly interpolates from @p a to @p b.
 * @param a Value returned when @p t is zero.
 * @param b Value returned when @p t is one.
 * @param t Blend amount; values outside zero through one extrapolate.
 * @return Interpolated or extrapolated vector.
 */
[[nodiscard]] Vec2 lerp(const Vec2& a, const Vec2& b, float t) noexcept;

/**
 * @brief Linearly interpolates from @p a to @p b.
 * @param a Value returned when @p t is zero.
 * @param b Value returned when @p t is one.
 * @param t Blend amount; values outside zero through one extrapolate.
 * @return Interpolated or extrapolated vector.
 */
[[nodiscard]] Vec3 lerp(const Vec3& a, const Vec3& b, float t) noexcept;

/**
 * @brief Returns the squared length without performing a square root.
 * @param vector Vector to measure.
 * @return Squared Euclidean length of @p vector.
 */
[[nodiscard]] float lengthSquared(const Vec2& vector) noexcept;

/**
 * @brief Returns the squared length without performing a square root.
 * @param vector Vector to measure.
 * @return Squared Euclidean length of @p vector.
 */
[[nodiscard]] float lengthSquared(const Vec3& vector) noexcept;

/**
 * @brief Returns the squared distance between two points.
 * @param first First point.
 * @param second Second point.
 * @return Squared Euclidean distance between the points.
 */
[[nodiscard]] float distanceSquared(const Vec2& first, const Vec2& second) noexcept;

/**
 * @brief Returns the squared distance between two points.
 * @param first First point.
 * @param second Second point.
 * @return Squared Euclidean distance between the points.
 */
[[nodiscard]] float distanceSquared(const Vec3& first, const Vec3& second) noexcept;

/**
 * @brief Returns a unit-length copy, or zero when the vector is too small.
 * @param vector Vector to normalize safely.
 * @param epsilon Length at or below which the vector is treated as zero.
 * @return Unit-length vector, or the zero vector when below @p epsilon.
 */
[[nodiscard]] Vec2 normalizedOrZero(const Vec2& vector, float epsilon = 0.000001F) noexcept;

/**
 * @brief Returns a unit-length copy, or zero when the vector is too small.
 * @param vector Vector to normalize safely.
 * @param epsilon Length at or below which the vector is treated as zero.
 * @return Unit-length vector, or the zero vector when below @p epsilon.
 */
[[nodiscard]] Vec3 normalizedOrZero(const Vec3& vector, float epsilon = 0.000001F) noexcept;

/**
 * @brief Returns the unit direction from @p from to @p to, or zero if they coincide.
 * @param from Starting point.
 * @param to Destination point.
 * @return Unit vector from @p from to @p to, or zero when they coincide.
 */
[[nodiscard]] Vec2 direction(const Vec2& from, const Vec2& to) noexcept;

/**
 * @brief Returns the unit direction from @p from to @p to, or zero if they coincide.
 * @param from Starting point.
 * @param to Destination point.
 * @return Unit vector from @p from to @p to, or zero when they coincide.
 */
[[nodiscard]] Vec3 direction(const Vec3& from, const Vec3& to) noexcept;

/**
 * @brief Moves toward @p target without travelling farther than @p maxDistanceDelta.
 * @param current Current position.
 * @param target Destination position.
 * @param maxDistanceDelta Non-negative maximum distance to move this call.
 * @return Position moved toward @p target by at most @p maxDistanceDelta.
 */
[[nodiscard]] Vec2 moveTowards(
    const Vec2& current,
    const Vec2& target,
    float maxDistanceDelta
) noexcept;

/**
 * @brief Moves toward @p target without travelling farther than @p maxDistanceDelta.
 * @param current Current position.
 * @param target Destination position.
 * @param maxDistanceDelta Non-negative maximum distance to move this call.
 * @return Position moved toward @p target by at most @p maxDistanceDelta.
 */
[[nodiscard]] Vec3 moveTowards(
    const Vec3& current,
    const Vec3& target,
    float maxDistanceDelta
) noexcept;

/**
 * @brief Returns the unsigned angle between two vectors in radians.
 * @param first First direction vector.
 * @param second Second direction vector.
 * @return Angle in radians, or zero if either vector has zero length.
 */
[[nodiscard]] float angleBetween(const Vec2& first, const Vec2& second) noexcept;

/**
 * @brief Returns the unsigned angle between two vectors in radians.
 * @param first First direction vector.
 * @param second Second direction vector.
 * @return Angle in radians, or zero if either vector has zero length.
 */
[[nodiscard]] float angleBetween(const Vec3& first, const Vec3& second) noexcept;

} // namespace vshade::math
