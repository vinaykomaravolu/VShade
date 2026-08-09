#pragma once

#include "math/matrix.hpp"

namespace vshade::renderer {

/** @brief Stores the view and projection matrices used to render a scene. */
class Camera final {
public:
    Camera() = default;

    /** @brief Creates a camera with an explicit projection matrix. */
    explicit Camera(const math::Mat4& projection);

    void setView(const math::Mat4& view);
    void setProjection(const math::Mat4& projection);

    /** @brief Sets a right-handed view matrix looking from @p eye toward @p target. */
    void lookAt(const math::Vec3& eye, const math::Vec3& target, const math::Vec3& up);

    /** @brief Replaces the projection with an OpenGL perspective projection. */
    void setPerspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane);

    /** @brief Replaces the projection with an OpenGL orthographic projection. */
    void setOrthographic(
        float left,
        float right,
        float bottom,
        float top,
        float nearPlane,
        float farPlane
    );

    [[nodiscard]] const math::Mat4& view() const noexcept;
    [[nodiscard]] const math::Mat4& projection() const noexcept;
    [[nodiscard]] math::Mat4 viewProjection() const;

private:
    math::Mat4 m_view{1.0F};
    math::Mat4 m_projection{1.0F};
};

} // namespace vshade::renderer
