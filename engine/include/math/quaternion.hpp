#pragma once

#include "math/vector.hpp"

#include <glm/gtc/quaternion.hpp>

namespace vshade::math {

/** @brief Floating-point quaternion used for 3D rotations. */
using Quat = glm::quat;

/** @brief Double-precision quaternion used for 3D rotations. */
using DQuat = glm::dquat;

/** @brief Returns the identity quaternion, which represents no rotation. */
[[nodiscard]] Quat identity() noexcept;

/**
 * @brief Creates a quaternion from XYZ Euler angles expressed in radians.
 */
[[nodiscard]] Quat fromEuler(const Vec3& eulerRadians);

/** @brief Converts a quaternion into XYZ Euler angles expressed in radians. */
[[nodiscard]] Vec3 toEuler(const Quat& quaternion);

/**
 * @brief Creates a quaternion from an axis and angle.
 * @param axis Rotation axis; it is normalized by the function.
 * @param angleRadians Rotation angle in radians.
 * @throws std::invalid_argument If @p axis has zero length.
 */
[[nodiscard]] Quat fromAxisAngle(const Vec3& axis, float angleRadians);

/**
 * @brief Returns a unit-length quaternion.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Quat normalize(const Quat& quaternion);

/**
 * @brief Returns the inverse rotation.
 * @throws std::invalid_argument If @p quaternion has zero length.
 */
[[nodiscard]] Quat inverse(const Quat& quaternion);

/**
 * @brief Spherically interpolates between two orientations.
 * @param t Blend amount clamped to the range zero through one.
 */
[[nodiscard]] Quat slerp(const Quat& start, const Quat& end, float t);

/** @brief Rotates @p vector by @p quaternion without modifying its length. */
[[nodiscard]] Vec3 rotate(const Quat& quaternion, const Vec3& vector);

/** @brief Returns the rotated engine-forward direction, whose base direction is -Z. */
[[nodiscard]] Vec3 forward(const Quat& rotation);

/** @brief Returns the rotated engine-right direction, whose base direction is +X. */
[[nodiscard]] Vec3 right(const Quat& rotation);

/** @brief Returns the rotated engine-up direction, whose base direction is +Y. */
[[nodiscard]] Vec3 up(const Quat& rotation);

} // namespace vshade::math
