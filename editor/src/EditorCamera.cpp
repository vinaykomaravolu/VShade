#include "EditorCamera.hpp"

#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <input/MouseCode.hpp>

#include <cmath>

#include <glm/gtc/matrix_inverse.hpp>

namespace editor {

EditorCamera::EditorCamera(
    const float verticalFovRadians,
    const float aspectRatio,
    const float nearClip,
    const float farClip
) : m_controller(m_camera),
    m_verticalFovRadians(verticalFovRadians),
    m_aspectRatio(aspectRatio),
    m_nearClip(nearClip),
    m_farClip(farClip) {
    m_camera.setPerspective(
        m_verticalFovRadians,
        m_aspectRatio,
        m_nearClip,
        m_farClip
    );
    m_camera.lookAt(
        {0.0F, 0.0F, 5.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );
    m_controller.syncFromCamera();
}

void EditorCamera::onUpdate(const float deltaTime) {
    using vshade::input::Input;
    using vshade::input::KeyCode;
    using vshade::input::MouseButton;

    m_looking = m_inputEnabled && Input::isMouseButtonDown(MouseButton::Right);
    const auto movementAxis = [this](const KeyCode positive, const KeyCode negative) {
        if (!m_looking) {
            return 0.0F;
        }
        return (Input::isKeyDown(positive) ? 1.0F : 0.0F)
            - (Input::isKeyDown(negative) ? 1.0F : 0.0F);
    };

    m_controller.update(deltaTime, {
        .forward = movementAxis(KeyCode::W, KeyCode::S),
        .right = movementAxis(KeyCode::D, KeyCode::A),
        .lookDelta = m_looking ? Input::mouseDelta() : vshade::math::Vec2{0.0F},
        .scrollDelta = 0.0F,
        .mouseLookActive = m_looking,
    });
}

void EditorCamera::setInputEnabled(const bool enabled) noexcept {
    m_inputEnabled = enabled;
    if (!enabled) {
        m_looking = false;
    }
}

void EditorCamera::setViewportSize(const float width, const float height) {
    if (!std::isfinite(width) || !std::isfinite(height)
        || width <= 0.0F || height <= 0.0F) {
        return;
    }

    const float aspectRatio = width / height;
    if (aspectRatio == m_aspectRatio) {
        return;
    }

    m_aspectRatio = aspectRatio;
    m_camera.setPerspective(
        m_verticalFovRadians,
        m_aspectRatio,
        m_nearClip,
        m_farClip
    );
}

void EditorCamera::focusOn(
    const vshade::math::Vec3& target,
    const float radius
) {
    if (!std::isfinite(target.x)
        || !std::isfinite(target.y)
        || !std::isfinite(target.z)
        || !std::isfinite(radius)
        || radius <= 0.0F) {
        return;
    }

    const vshade::math::Mat4 inverseView = glm::inverse(m_camera.view());
    vshade::math::Vec3 forward = vshade::math::normalizedOrZero(
        -vshade::math::Vec3(inverseView[2])
    );
    if (vshade::math::lengthSquared(forward) == 0.0F) {
        forward = {0.0F, 0.0F, -1.0F};
    }
    vshade::math::Vec3 up = vshade::math::normalizedOrZero(
        vshade::math::Vec3(inverseView[1])
    );
    if (vshade::math::lengthSquared(up) == 0.0F) {
        up = {0.0F, 1.0F, 0.0F};
    }

    const float halfFov = m_verticalFovRadians * 0.5F;
    const float framingDistance = std::max(
        radius / std::sin(halfFov) * 1.2F,
        m_nearClip + radius
    );
    m_camera.lookAt(target - forward * framingDistance, target, up);
    m_controller.syncFromCamera();
}

void EditorCamera::setMovementSpeed(const float speed) {
    m_controller.setMovementSpeed(speed);
}

bool EditorCamera::isLooking() const noexcept {
    return m_looking;
}

float EditorCamera::movementSpeed() const noexcept {
    return m_controller.movementSpeed();
}

const vshade::renderer::Camera& EditorCamera::camera() const noexcept {
    return m_camera;
}

const vshade::math::Mat4& EditorCamera::viewMatrix() const noexcept {
    return m_camera.view();
}

const vshade::math::Mat4& EditorCamera::projectionMatrix() const noexcept {
    return m_camera.projection();
}

} // namespace editor
