#pragma once

#include <math/Matrix.hpp>
#include <renderer/Camera.hpp>
#include <renderer/CameraController.hpp>

namespace editor {

class EditorCamera final {
public:
    EditorCamera(
        float verticalFovRadians,
        float aspectRatio,
        float nearClip,
        float farClip
    );

    void onUpdate(float deltaTime);
    void setInputEnabled(bool enabled) noexcept;
    void setViewportSize(float width, float height);
    /** Frames a world-space target while preserving the current view direction. */
    void focusOn(const vshade::math::Vec3& target, float radius);
    void setMovementSpeed(float speed);

    [[nodiscard]] bool isLooking() const noexcept;
    [[nodiscard]] float movementSpeed() const noexcept;
    [[nodiscard]] const vshade::renderer::Camera& camera() const noexcept;
    [[nodiscard]] const vshade::math::Mat4& viewMatrix() const noexcept;
    [[nodiscard]] const vshade::math::Mat4& projectionMatrix() const noexcept;

private:
    vshade::renderer::Camera m_camera;
    vshade::renderer::CameraController m_controller;
    float m_verticalFovRadians = 0.785398163F;
    float m_aspectRatio = 16.0F / 9.0F;
    float m_nearClip = 0.1F;
    float m_farClip = 1000.0F;
    bool m_inputEnabled = false;
    bool m_looking = false;
};

} // namespace editor
