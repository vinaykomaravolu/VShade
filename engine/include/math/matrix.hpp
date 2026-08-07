#pragma once

#include "math/quaternion.hpp"
#include "math/vector.hpp"

#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace vshade::math {

/** @brief Two-by-two floating-point matrix. */
using Mat2 = glm::mat2;

/** @brief Three-by-three floating-point matrix. */
using Mat3 = glm::mat3;

/** @brief Four-by-four floating-point matrix. */
using Mat4 = glm::mat4;

/** @brief Two-by-two double-precision matrix. */
using DMat2 = glm::dmat2;

/** @brief Three-by-three double-precision matrix. */
using DMat3 = glm::dmat3;

/** @brief Four-by-four double-precision matrix. */
using DMat4 = glm::dmat4;

/** @brief Returns a four-by-four identity matrix. */
[[nodiscard]] Mat4 identityMatrix() noexcept;

/** @brief Creates a matrix that moves positions by @p translation. */
[[nodiscard]] Mat4 translationMatrix(const Vec3& translation);

/** @brief Creates a matrix from a quaternion rotation. */
[[nodiscard]] Mat4 rotationMatrix(const Quat& rotation);

/** @brief Creates a matrix that scales each local axis independently. */
[[nodiscard]] Mat4 scaleMatrix(const Vec3& scale);

/**
 * @brief Combines translation, rotation, and scale into a model matrix.
 *
 * The returned matrix applies scale first, rotation second, and translation
 * last when multiplied by a column vector.
 */
[[nodiscard]] Mat4 composeTransform(
    const Vec3& translation,
    const Quat& rotation,
    const Vec3& scale
);

/**
 * @brief Creates an OpenGL perspective projection matrix.
 * @param vertical_fov_radians Vertical field of view in radians.
 * @param aspect_ratio Viewport width divided by viewport height.
 * @param near_plane Positive distance to the near clipping plane.
 * @param far_plane Distance to the far clipping plane, greater than near_plane.
 */
[[nodiscard]] Mat4 perspective(
    float vertical_fov_radians,
    float aspect_ratio,
    float near_plane,
    float far_plane
);

/** @brief Creates an OpenGL orthographic projection matrix. */
[[nodiscard]] Mat4 orthographic(
    float left,
    float right,
    float bottom,
    float top,
    float near_plane,
    float far_plane
);

/**
 * @brief Creates a right-handed view matrix looking from @p eye toward @p target.
 * @param up Approximate world-up direction used to orient the camera.
 */
[[nodiscard]] Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

} // namespace vshade::math
