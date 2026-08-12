#include "renderer/cameracontroller.hpp"

#include "input/input.hpp"
#include "input/keycode.hpp"
#include "input/mousecode.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <glm/common.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace vshade::renderer {
namespace {

constexpr float maximumPitch = 1.55334306F;

void validateNonNegativeFinite(const float value, const char* message) {
    if (!std::isfinite(value) || value < 0.0F) {
        throw std::invalid_argument(message);
    }
}

[[nodiscard]] math::Vec3 cameraForward(const float yaw, const float pitch) {
    const float pitchCosine = std::cos(pitch);
    return math::normalize({
        pitchCosine * std::cos(yaw),
        std::sin(pitch),
        pitchCosine * std::sin(yaw),
    });
}

} // namespace

FlyCameraController::FlyCameraController(
    Camera& camera,
    const FlyCameraControllerConfig config
) : m_camera(&camera), m_config(config) {
    validateNonNegativeFinite(
        m_config.movementSpeed,
        "Camera movement speed must be finite and non-negative"
    );
    validateNonNegativeFinite(
        m_config.mouseSensitivity,
        "Camera mouse sensitivity must be finite and non-negative"
    );
    validateNonNegativeFinite(
        m_config.scrollSpeedStep,
        "Camera scroll step must be finite and non-negative"
    );

    const math::Mat4 inverseView = glm::inverse(camera.view());
    m_position = math::Vec3(inverseView[3]);
    const math::Vec3 forward = math::normalizedOrZero(-math::Vec3(inverseView[2]));
    if (math::lengthSquared(forward) > 0.0F) {
        m_pitch = std::asin(glm::clamp(forward.y, -1.0F, 1.0F));
        m_yaw = std::atan2(forward.z, forward.x);
    }
}

void FlyCameraController::update(const float deltaTime) {
    validateNonNegativeFinite(deltaTime, "Camera delta time must be finite and non-negative");

    const float scroll = input::Input::scrollDelta().y;
    m_config.movementSpeed = std::max(
        0.0F,
        m_config.movementSpeed + scroll * m_config.scrollSpeedStep
    );

    const bool mouseLookActive = !m_config.requireRightMouseButton ||
        input::Input::isMouseButtonDown(input::MouseButton::Right);
    if (mouseLookActive) {
        const math::Vec2 mouseDelta = input::Input::mouseDelta();
        m_yaw += mouseDelta.x * m_config.mouseSensitivity;
        m_pitch = glm::clamp(
            m_pitch - mouseDelta.y * m_config.mouseSensitivity,
            -maximumPitch,
            maximumPitch
        );
    }

    const math::Vec3 forward = cameraForward(m_yaw, m_pitch);
    const math::Vec3 right = math::normalize(math::cross(forward, {0.0F, 1.0F, 0.0F}));
    math::Vec3 movement{0.0F};

    if (input::Input::isKeyDown(input::KeyCode::W)) {
        movement += forward;
    }
    if (input::Input::isKeyDown(input::KeyCode::S)) {
        movement -= forward;
    }
    if (input::Input::isKeyDown(input::KeyCode::D)) {
        movement += right;
    }
    if (input::Input::isKeyDown(input::KeyCode::A)) {
        movement -= right;
    }

    movement = math::normalizedOrZero(movement);
    m_position += movement * m_config.movementSpeed * deltaTime;
    m_camera->lookAt(m_position, m_position + forward, {0.0F, 1.0F, 0.0F});
}

float FlyCameraController::movementSpeed() const noexcept {
    return m_config.movementSpeed;
}

void FlyCameraController::setMovementSpeed(const float speed) {
    validateNonNegativeFinite(speed, "Camera movement speed must be finite and non-negative");
    m_config.movementSpeed = speed;
}

} // namespace vshade::renderer
