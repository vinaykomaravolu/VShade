#include "renderer/camera.hpp"

namespace vshade::renderer {

Camera::Camera(const math::Mat4& projection)
    : m_projection(projection) {}

void Camera::setView(const math::Mat4& view) {
    m_view = view;
}

void Camera::setProjection(const math::Mat4& projection) {
    m_projection = projection;
}

void Camera::lookAt(const math::Vec3& eye, const math::Vec3& target, const math::Vec3& up) {
    m_view = math::lookAt(eye, target, up);
}

void Camera::setPerspective(
    const float verticalFovRadians,
    const float aspectRatio,
    const float nearPlane,
    const float farPlane
) {
    m_projection = math::perspective(verticalFovRadians, aspectRatio, nearPlane, farPlane);
}

void Camera::setOrthographic(
    const float left,
    const float right,
    const float bottom,
    const float top,
    const float nearPlane,
    const float farPlane
) {
    m_projection = math::orthographic(left, right, bottom, top, nearPlane, farPlane);
}

const math::Mat4& Camera::view() const noexcept {
    return m_view;
}

const math::Mat4& Camera::projection() const noexcept {
    return m_projection;
}

math::Mat4 Camera::viewProjection() const {
    return m_projection * m_view;
}

} // namespace vshade::renderer
