#include "math/Transform.hpp"

namespace vshade::math {

Transform::Transform(const Vec3& position, const Quat& rotation, const Vec3& scale)
    : m_position(position),
      m_rotation(vshade::math::normalize(rotation)),
      m_scale(scale) {}

const Vec3& Transform::position() const noexcept {
    return m_position;
}

const Quat& Transform::rotation() const noexcept {
    return m_rotation;
}

const Vec3& Transform::scale() const noexcept {
    return m_scale;
}

void Transform::setPosition(const Vec3& position) noexcept {
    m_position = position;
}

void Transform::setRotation(const Quat& rotation) {
    m_rotation = vshade::math::normalize(rotation);
}

void Transform::setScale(const Vec3& scale) noexcept {
    m_scale = scale;
}

void Transform::translate(const Vec3& offset) noexcept {
    m_position += offset;
}

void Transform::rotate(const Quat& rotation) {
    m_rotation = vshade::math::normalize(m_rotation * vshade::math::normalize(rotation));
}

Mat4 Transform::matrix() const {
    return composeTransform(m_position, m_rotation, m_scale);
}

Vec3 Transform::transformPoint(const Vec3& point) const {
    return m_position + vshade::math::rotate(m_rotation, m_scale * point);
}

Vec3 Transform::transformVector(const Vec3& vector) const {
    return vshade::math::rotate(m_rotation, m_scale * vector);
}

Vec3 Transform::transformDirection(const Vec3& direction) const {
    return vshade::math::rotate(m_rotation, direction);
}

Vec3 Transform::forward() const {
    return vshade::math::forward(m_rotation);
}

Vec3 Transform::right() const {
    return vshade::math::right(m_rotation);
}

Vec3 Transform::up() const {
    return vshade::math::up(m_rotation);
}

} // namespace vshade::math
