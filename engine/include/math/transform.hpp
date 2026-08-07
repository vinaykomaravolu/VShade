#pragma once

#include "math/matrix.hpp"
#include "math/quaternion.hpp"
#include "math/vector.hpp"

namespace vshade::math {

/**
 * @brief Position, rotation, and scale for an object in 3D space.
 *
 * The generated matrix applies scale first, rotation second, and position
 * last, which is the usual local-to-parent transform order.
 */
class Transform {
public:
    /** @brief Creates an identity transform. */
    Transform() = default;

    /** @brief Creates a transform from a position, rotation, and scale. */
    Transform(
        const Vec3& position,
        const Quat& rotation = Quat{1.0F, 0.0F, 0.0F, 0.0F},
        const Vec3& scale = Vec3{1.0F}
    );

    /** @brief Returns the position in parent space. */
    [[nodiscard]] const Vec3& position() const noexcept;

    /** @brief Returns the local orientation. */
    [[nodiscard]] const Quat& rotation() const noexcept;

    /** @brief Returns the scale on each local axis. */
    [[nodiscard]] const Vec3& scale() const noexcept;

    /** @brief Replaces the position in parent space. */
    void setPosition(const Vec3& position) noexcept;

    /** @brief Replaces and normalizes the local orientation. */
    void setRotation(const Quat& rotation);

    /** @brief Replaces the scale on each local axis. */
    void setScale(const Vec3& scale) noexcept;

    /** @brief Adds a parent-space offset to the current position. */
    void translate(const Vec3& offset) noexcept;

    /** @brief Applies and normalizes an additional local-space rotation. */
    void rotate(const Quat& rotation);

    /** @brief Builds the local-to-parent transformation matrix. */
    [[nodiscard]] Mat4 matrix() const;

    /** @brief Converts a local point using scale, rotation, and translation. */
    [[nodiscard]] Vec3 transformPoint(const Vec3& point) const;

    /** @brief Converts a local vector using scale and rotation, but not translation. */
    [[nodiscard]] Vec3 transformVector(const Vec3& vector) const;

    /** @brief Converts a local direction using rotation only. */
    [[nodiscard]] Vec3 transformDirection(const Vec3& direction) const;

    /** @brief Returns this transform's forward direction in parent space. */
    [[nodiscard]] Vec3 forward() const;

    /** @brief Returns this transform's right direction in parent space. */
    [[nodiscard]] Vec3 right() const;

    /** @brief Returns this transform's up direction in parent space. */
    [[nodiscard]] Vec3 up() const;

private:
    Vec3 m_position{0.0F};
    Quat m_rotation{1.0F, 0.0F, 0.0F, 0.0F};
    Vec3 m_scale{1.0F};
};

} // namespace vshade::math
