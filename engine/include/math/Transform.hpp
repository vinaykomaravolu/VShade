#pragma once

#include "math/Matrix.hpp"
#include "math/Quaternion.hpp"
#include "math/Vector.hpp"

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

    /**
     * @brief Creates a transform from a position, rotation, and scale.
     * @param position Initial position in parent space.
     * @param rotation Initial local orientation.
     * @param scale Initial scale along each local axis.
     * @throws std::invalid_argument If @p rotation has zero length.
     */
    Transform(
        const Vec3& position,
        const Quat& rotation = Quat{1.0F, 0.0F, 0.0F, 0.0F},
        const Vec3& scale = Vec3{1.0F}
    );

    /**
     * @brief Returns the position in parent space.
     * @return Read-only reference to the stored position.
     */
    [[nodiscard]] const Vec3& position() const noexcept;

    /**
     * @brief Returns the local orientation.
     * @return Read-only reference to the normalized orientation.
     */
    [[nodiscard]] const Quat& rotation() const noexcept;

    /**
     * @brief Returns the scale on each local axis.
     * @return Read-only reference to the stored scale.
     */
    [[nodiscard]] const Vec3& scale() const noexcept;

    /**
     * @brief Replaces the position in parent space.
     * @param position New position in parent space.
     */
    void setPosition(const Vec3& position) noexcept;

    /**
     * @brief Replaces and normalizes the local orientation.
     * @param rotation New local orientation.
     * @throws std::invalid_argument If @p rotation has zero length.
     */
    void setRotation(const Quat& rotation);

    /**
     * @brief Replaces the scale on each local axis.
     * @param scale New scale along each local axis.
     */
    void setScale(const Vec3& scale) noexcept;

    /**
     * @brief Adds a parent-space offset to the current position.
     * @param offset Parent-space displacement to add.
     */
    void translate(const Vec3& offset) noexcept;

    /**
     * @brief Applies and normalizes an additional local-space rotation.
     * @param rotation Local-space rotation to apply.
     * @throws std::invalid_argument If @p rotation has zero length.
     */
    void rotate(const Quat& rotation);

    /**
     * @brief Builds the local-to-parent transformation matrix.
     * @return Matrix combining this transform's position, rotation, and scale.
     */
    [[nodiscard]] Mat4 matrix() const;

    /**
     * @brief Converts a local point using scale, rotation, and translation.
     * @param point Point expressed in local space.
     * @return Point transformed into parent space.
     */
    [[nodiscard]] Vec3 transformPoint(const Vec3& point) const;

    /**
     * @brief Converts a local vector using scale and rotation, but not translation.
     * @param vector Vector expressed in local space.
     * @return Vector transformed into parent space.
     */
    [[nodiscard]] Vec3 transformVector(const Vec3& vector) const;

    /**
     * @brief Converts a local direction using rotation only.
     * @param direction Direction expressed in local space.
     * @return Direction rotated into parent space.
     */
    [[nodiscard]] Vec3 transformDirection(const Vec3& direction) const;

    /**
     * @brief Returns this transform's forward direction in parent space.
     * @return Unit direction corresponding to local negative Z.
     */
    [[nodiscard]] Vec3 forward() const;

    /**
     * @brief Returns this transform's right direction in parent space.
     * @return Unit direction corresponding to local positive X.
     */
    [[nodiscard]] Vec3 right() const;

    /**
     * @brief Returns this transform's up direction in parent space.
     * @return Unit direction corresponding to local positive Y.
     */
    [[nodiscard]] Vec3 up() const;

private:
    Vec3 m_position{0.0F};
    Quat m_rotation{1.0F, 0.0F, 0.0F, 0.0F};
    Vec3 m_scale{1.0F};
};

} // namespace vshade::math
