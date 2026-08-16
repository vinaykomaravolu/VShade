#pragma once

#include "math/Vector.hpp"

#include <glm/gtc/quaternion.hpp>

namespace vshade::math {

/** @brief Floating-point quaternion used for 3D rotations. */
using Quat = glm::quat;

/** @brief Double-precision quaternion used for 3D rotations. */
using DQuat = glm::dquat;

/**
 * @brief Returns the identity quaternion, which represents no rotation.
 * @return Unit quaternion with a real component of one.
 */
[[nodiscard]] Quat identity() noexcept;

/**
 * @brief Creates a quaternion from XYZ Euler angles expressed in radians.
 * @param eulerRadians Rotation around the X, Y, and Z axes in radians.
 * @return Normalized quaternion representing the supplied Euler rotation.
 */
[[nodiscard]] Quat fromEuler(const Vec3& eulerRadians);

/**
 * @brief Converts a quaternion into XYZ Euler angles expressed in radians.
 * @param quaternion Orientation to convert.
 * @return XYZ Euler angles in radians.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Vec3 toEuler(const Quat& quaternion);

/**
 * @brief Creates a quaternion from an axis and angle.
 * @param axis Rotation axis; it is normalized by the function.
 * @param angleRadians Rotation angle in radians.
 * @return Normalized quaternion representing the axis-angle rotation.
 * @throws std::invalid_argument If @p axis has zero length.
 */
[[nodiscard]] Quat fromAxisAngle(const Vec3& axis, float angleRadians);

/** @brief Rotates engine forward (-Z) toward a direction with a chosen up axis. */
[[nodiscard]] Quat lookRotation(
    const Vec3& direction,
    const Vec3& up = {0.0F, 1.0F, 0.0F}
);

/**
 * @brief Returns a unit-length quaternion.
 * @param quaternion Quaternion to normalize.
 * @return Unit-length copy of @p quaternion.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Quat normalize(const Quat& quaternion);

/**
 * @brief Returns the inverse rotation.
 * @param quaternion Quaternion to invert.
 * @return Normalized inverse of @p quaternion.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Quat inverse(const Quat& quaternion);

/**
 * @brief Spherically interpolates between two orientations.
 * @param start Orientation returned when @p t is zero.
 * @param end Orientation returned when @p t is one.
 * @param t Blend amount clamped to the range zero through one.
 * @return Normalized interpolated orientation.
 * @throws std::invalid_argument If either quaternion has zero length.
 */
[[nodiscard]] Quat slerp(const Quat& start, const Quat& end, float t);

/**
 * @brief Rotates @p vector by @p quaternion without modifying its length.
 * @param quaternion Rotation to apply.
 * @param vector Vector to rotate.
 * @return Rotated copy of @p vector.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Vec3 rotate(const Quat& quaternion, const Vec3& vector);

/**
 * @brief Returns the rotated engine-forward direction, whose base direction is -Z.
 * @param rotation Orientation used to rotate the base direction.
 * @return Unit forward direction after rotation.
 * @throws std::invalid_argument If @p rotation has zero length.
 */
[[nodiscard]] Vec3 forward(const Quat& rotation);

/**
 * @brief Returns the rotated engine-right direction, whose base direction is +X.
 * @param rotation Orientation used to rotate the base direction.
 * @return Unit right direction after rotation.
 * @throws std::invalid_argument If @p rotation has zero length.
 */
[[nodiscard]] Vec3 right(const Quat& rotation);

/**
 * @brief Returns the rotated engine-up direction, whose base direction is +Y.
 * @param rotation Orientation used to rotate the base direction.
 * @return Unit up direction after rotation.
 * @throws std::invalid_argument If @p rotation has zero length.
 */
[[nodiscard]] Vec3 up(const Quat& rotation);

} // namespace vshade::math
