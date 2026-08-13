#include "renderer/camera.hpp"

#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <string>

#include <glm/gtc/matrix_inverse.hpp>

namespace vshade::renderer {
namespace {

void validateMatrix(const math::Mat4& matrix, const char* name) {
    for (glm::length_t column = 0; column < 4; ++column) {
        for (glm::length_t row = 0; row < 4; ++row) {
            if (!std::isfinite(matrix[column][row])) {
                throw std::invalid_argument(std::string(name) + " must contain only finite values");
            }
        }
    }
}

void validateFinite(const float value, const char* name) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string(name) + " must be finite");
    }
}

void validateVector(const math::Vec3& vector, const char* name) {
    validateFinite(vector.x, name);
    validateFinite(vector.y, name);
    validateFinite(vector.z, name);
}

} // namespace

Camera::Camera(const math::Mat4& projection)
    : m_projection(projection) {
    validateMatrix(projection, "Camera projection");
}

void Camera::setView(const math::Mat4& view) {
    validateMatrix(view, "Camera view");
    const float determinant = glm::determinant(view);
    if (!std::isfinite(determinant) || std::abs(determinant) <= 0.000001F) {
        throw std::invalid_argument("Camera view matrix must be invertible");
    }
    m_view = view;
}

void Camera::setProjection(const math::Mat4& projection) {
    validateMatrix(projection, "Camera projection");
    m_projection = projection;
}

void Camera::lookAt(const math::Vec3& eye, const math::Vec3& target, const math::Vec3& up) {
    validateVector(eye, "Camera eye");
    validateVector(target, "Camera target");
    validateVector(up, "Camera up vector");
    const math::Vec3 direction = target - eye;
    if (math::lengthSquared(direction) <= 0.000001F) {
        throw std::invalid_argument("Camera eye and target must differ");
    }
    if (math::lengthSquared(up) <= 0.000001F ||
        math::lengthSquared(math::cross(direction, up)) <= 0.000001F) {
        throw std::invalid_argument("Camera up vector must not be zero or parallel to its direction");
    }
    m_view = math::lookAt(eye, target, up);
}

void Camera::setPerspective(
    const float verticalFovRadians,
    const float aspectRatio,
    const float nearPlane,
    const float farPlane
) {
    validateFinite(verticalFovRadians, "Camera vertical field of view");
    validateFinite(aspectRatio, "Camera aspect ratio");
    validateFinite(nearPlane, "Camera near plane");
    validateFinite(farPlane, "Camera far plane");
    if (verticalFovRadians <= 0.0F || verticalFovRadians >= std::numbers::pi_v<float>) {
        throw std::invalid_argument("Camera vertical field of view must be between zero and pi");
    }
    if (aspectRatio <= 0.0F) {
        throw std::invalid_argument("Camera aspect ratio must be positive");
    }
    if (nearPlane <= 0.0F || farPlane <= nearPlane) {
        throw std::invalid_argument("Camera perspective planes require 0 < near < far");
    }
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
    validateFinite(left, "Camera left plane");
    validateFinite(right, "Camera right plane");
    validateFinite(bottom, "Camera bottom plane");
    validateFinite(top, "Camera top plane");
    validateFinite(nearPlane, "Camera near plane");
    validateFinite(farPlane, "Camera far plane");
    if (left == right || bottom == top || nearPlane == farPlane) {
        throw std::invalid_argument("Camera orthographic bounds must differ on every axis");
    }
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
