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

    /**
     * @brief Applies WASD movement, mouse look, and scroll-speed changes.
     * @param deltaTime Seconds elapsed since the previous frame.
     * @throws std::invalid_argument If @p deltaTime is negative or not finite.
     */
    void update(float deltaTime);

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
    Camera* m_camera = nullptr;
    CameraControllerConfig m_config{};
    math::Vec3 m_position{0.0F};
    float m_yaw = 0.0F;
    float m_pitch = 0.0F;
};

} // namespace vshade::renderer
