#pragma once

#include "math/Matrix.hpp"

namespace vshade::renderer {

/** @brief Stores the view and projection matrices used to render a scene. */
class Camera final {
public:
    Camera() = default;

    /**
     * @brief Creates a camera with an explicit projection matrix.
     * @param projection Initial projection matrix.
     */
    explicit Camera(const math::Mat4& projection);

    /**
     * @brief Replaces the camera view matrix.
     * @param view New world-to-view transformation.
     */
    void setView(const math::Mat4& view);

    /**
     * @brief Replaces the camera projection matrix.
     * @param projection New view-to-clip transformation.
     */
    void setProjection(const math::Mat4& projection);

    /**
     * @brief Sets a right-handed view matrix looking from @p eye toward @p target.
     * @param eye Camera position in world space.
     * @param target World-space point the camera looks toward.
     * @param up Approximate world-up direction.
     * @throws std::invalid_argument If values are non-finite or the view basis is degenerate.
     */
    void lookAt(const math::Vec3& eye, const math::Vec3& target, const math::Vec3& up);

    /**
     * @brief Replaces the projection with an OpenGL perspective projection.
     * @param verticalFovRadians Vertical field of view in radians.
     * @param aspectRatio Viewport width divided by height.
     * @param nearPlane Positive distance to the near clipping plane.
     * @param farPlane Distance to the far clipping plane.
     * @throws std::invalid_argument If values are non-finite or projection bounds are invalid.
     */
    void setPerspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane);

    /**
     * @brief Replaces the projection with an OpenGL orthographic projection.
     * @param left Coordinate of the left clipping plane.
     * @param right Coordinate of the right clipping plane.
     * @param bottom Coordinate of the bottom clipping plane.
     * @param top Coordinate of the top clipping plane.
     * @param nearPlane Distance to the near clipping plane.
     * @param farPlane Distance to the far clipping plane.
     * @throws std::invalid_argument If values are non-finite or any axis has equal bounds.
     */
    void setOrthographic(
        float left,
        float right,
        float bottom,
        float top,
        float nearPlane,
        float farPlane
    );

    /**
     * @brief Returns the world-to-view transformation.
     * @return Read-only reference to the view matrix.
     */
    [[nodiscard]] const math::Mat4& view() const noexcept;

    /**
     * @brief Returns the view-to-clip transformation.
     * @return Read-only reference to the projection matrix.
     */
    [[nodiscard]] const math::Mat4& projection() const noexcept;

    /**
     * @brief Combines the projection and view matrices.
     * @return Matrix that transforms world-space positions into clip space.
     */
    [[nodiscard]] math::Mat4 viewProjection() const;

private:
    math::Mat4 m_view{1.0F};
    math::Mat4 m_projection{1.0F};
};

} // namespace vshade::renderer
