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

CameraController::CameraController(
    Camera& camera,
    const CameraControllerConfig config
) : m_camera(camera), m_config(config) {
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

    syncFromCamera();
}

void CameraController::syncFromCamera() {
    const math::Mat4 inverseView = glm::inverse(m_camera.view());
    m_position = math::Vec3(inverseView[3]);
    const math::Vec3 forward = math::normalizedOrZero(-math::Vec3(inverseView[2]));
    if (math::lengthSquared(forward) > 0.0F) {
        m_pitch = std::asin(glm::clamp(forward.y, -1.0F, 1.0F));
        m_yaw = std::atan2(forward.z, forward.x);
    }
}

void CameraController::update(const float deltaTime) {
    const bool mouseLookActive = !m_config.requireRightMouseButton ||
        input::Input::isMouseButtonDown(input::MouseButton::Right);
    update(deltaTime, {
        .forward = (input::Input::isKeyDown(input::KeyCode::W) ? 1.0F : 0.0F) -
            (input::Input::isKeyDown(input::KeyCode::S) ? 1.0F : 0.0F),
        .right = (input::Input::isKeyDown(input::KeyCode::D) ? 1.0F : 0.0F) -
            (input::Input::isKeyDown(input::KeyCode::A) ? 1.0F : 0.0F),
        .lookDelta = input::Input::mouseDelta(),
        .scrollDelta = input::Input::scrollDelta().y,
        .mouseLookActive = mouseLookActive,
    });
}

void CameraController::update(
    const float deltaTime,
    const CameraControllerInput& inputState
) {
    validateNonNegativeFinite(deltaTime, "Camera delta time must be finite and non-negative");
    if (!std::isfinite(inputState.forward) || !std::isfinite(inputState.right) ||
        !std::isfinite(inputState.lookDelta.x) || !std::isfinite(inputState.lookDelta.y) ||
        !std::isfinite(inputState.scrollDelta)) {
        throw std::invalid_argument("Camera input values must be finite");
    }
    if (std::abs(inputState.forward) > 1.0F || std::abs(inputState.right) > 1.0F) {
        throw std::invalid_argument("Camera movement input must be between negative one and one");
    }

    m_config.movementSpeed = std::max(
        0.0F,
        m_config.movementSpeed + inputState.scrollDelta * m_config.scrollSpeedStep
    );

    if (inputState.mouseLookActive) {
        m_yaw += inputState.lookDelta.x * m_config.mouseSensitivity;
        m_pitch = glm::clamp(
            m_pitch - inputState.lookDelta.y * m_config.mouseSensitivity,
            -maximumPitch,
            maximumPitch
        );
    }

    const math::Vec3 forward = cameraForward(m_yaw, m_pitch);
    const math::Vec3 right = math::normalize(math::cross(forward, {0.0F, 1.0F, 0.0F}));
    math::Vec3 movement{0.0F};

    movement += forward * inputState.forward;
    movement += right * inputState.right;

    movement = math::normalizedOrZero(movement);
    m_position += movement * m_config.movementSpeed * deltaTime;
    m_camera.lookAt(m_position, m_position + forward, {0.0F, 1.0F, 0.0F});
}

const math::Vec3& CameraController::position() const noexcept {
    return m_position;
}

float CameraController::movementSpeed() const noexcept {
    return m_config.movementSpeed;
}

void CameraController::setMovementSpeed(const float speed) {
    validateNonNegativeFinite(speed, "Camera movement speed must be finite and non-negative");
    m_config.movementSpeed = speed;
}

} // namespace vshade::renderer
