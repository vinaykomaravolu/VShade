#pragma once

#include "math/vector.hpp"
#include "renderer/camera.hpp"

namespace vshade::renderer {

/** @brief Configuration for WASD and mouse-look camera movement. */
struct CameraControllerConfig {
    /** @brief Initial movement speed in world units per second. */
    float movementSpeed = 5.0F;
    /** @brief Mouse-look sensitivity in radians per input unit. */
    float mouseSensitivity = 0.002F;
    /** @brief Amount added to movement speed for each scroll step. */
    float scrollSpeedStep = 0.5F;
    /** @brief Whether mouse-look is active only while the right mouse button is held. */
    bool requireRightMouseButton = true;
};

/** @brief One frame of deterministic fly-camera input. */
struct CameraControllerInput {
    /** @brief Forward/back input in the range -1 through 1. */
    float forward = 0.0F;
    /** @brief Right/left input in the range -1 through 1. */
    float right = 0.0F;
    /** @brief Cursor movement in input units for this frame. */
    math::Vec2 lookDelta{0.0F};
    /** @brief Vertical scroll input used to adjust movement speed. */
    float scrollDelta = 0.0F;
    /** @brief Whether lookDelta should rotate the camera this frame. */
    bool mouseLookActive = false;
};

/** @brief Provides free-flying WASD and mouse-look control for a camera. */
class CameraController final {
public:
    /**
     * @brief Creates a controller for an existing camera.
     * @param camera Camera whose view is updated by input.
     * @param config Movement, look, and scroll sensitivity settings.
     * @throws std::invalid_argument If a configuration value is negative or not finite.
     */
    explicit CameraController(
        Camera& camera,
        CameraControllerConfig config = {}
    );

    CameraController(const CameraController&) = delete;
    CameraController& operator=(const CameraController&) = delete;
    CameraController(CameraController&&) = delete;
    CameraController& operator=(CameraController&&) = delete;

    /**
     * @brief Applies WASD movement, mouse look, and scroll-speed changes.
     * @param deltaTime Seconds elapsed since the previous frame.
     * @throws std::invalid_argument If @p deltaTime is negative or not finite.
     */
    void update(float deltaTime);

    /**
     * @brief Updates from an explicit input snapshot for tests or custom input systems.
     * @param deltaTime Seconds elapsed since the previous frame.
     * @param input Movement, look, and scroll values for this frame.
     */
    void update(float deltaTime, const CameraControllerInput& input);

    /** @brief Rebuilds the controller pose from the camera's current view matrix. */
    void syncFromCamera();

    /** @brief Returns the controller's current world-space position. */
    [[nodiscard]] const math::Vec3& position() const noexcept;

    /**
     * @brief Returns the current movement speed.
     * @return Movement speed in world units per second.
     */
    [[nodiscard]] float movementSpeed() const noexcept;

    /**
     * @brief Replaces the movement speed.
     * @param speed Non-negative world units travelled per second.
     * @throws std::invalid_argument If @p speed is negative or not finite.
     */
    void setMovementSpeed(float speed);

private:
    Camera& m_camera;
    CameraControllerConfig m_config{};
    math::Vec3 m_position{0.0F};
    float m_yaw = 0.0F;
    float m_pitch = 0.0F;
};

} // namespace vshade::renderer
