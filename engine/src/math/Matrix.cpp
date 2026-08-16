#include "math/Matrix.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace vshade::math {

Mat4 identityMatrix() noexcept {
    return Mat4{1.0F};
}

Mat4 translationMatrix(const Vec3& translation) {
    return glm::translate(Mat4{1.0F}, translation);
}

Mat4 rotationMatrix(const Quat& rotation) {
    return glm::mat4_cast(rotation);
}

Mat4 scaleMatrix(const Vec3& scale) {
    return glm::scale(Mat4{1.0F}, scale);
}

Mat4 composeTransform(
    const Vec3& translation,
    const Quat& rotation,
    const Vec3& scale
) {
    return translationMatrix(translation) * rotationMatrix(rotation) * scaleMatrix(scale);
}

Mat4 perspective(
    const float vertical_fov_radians,
    const float aspect_ratio,
    const float near_plane,
    const float far_plane
) {
    return glm::perspective(vertical_fov_radians, aspect_ratio, near_plane, far_plane);
}

Mat4 orthographic(
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float near_plane,
    const float far_plane
) {
    return glm::ortho(left, right, bottom, top, near_plane, far_plane);
}

Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    return glm::lookAt(eye, target, up);
}

} // namespace vshade::math
