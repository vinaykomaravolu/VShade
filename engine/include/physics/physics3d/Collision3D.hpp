#pragma once

#include "math/Vector.hpp"
#include "physics/PhysicsTypes.hpp"

#include <functional>

namespace vshade::physics {

/** @brief Result returned by a successful three-dimensional ray cast. */
struct RaycastHit3D {
    BodyId3D body;
    math::Vec3 point{0.0F};
    math::Vec3 normal{0.0F};
    float fraction = 0.0F;
};

/** @brief Ray query expressed as an origin and finite displacement. */
struct RaycastQuery3D {
    math::Vec3 origin{0.0F};
    math::Vec3 displacement{0.0F};
    CollisionFilter filter;
};

/** @brief Engine-facing contact notification from Jolt. */
struct ContactEvent3D {
    BodyId3D firstBody;
    BodyId3D secondBody;
    math::Vec3 normal{0.0F};
    math::Vec3 point{0.0F};
    ContactPhase phase = ContactPhase::Began;
};

using ContactListener3D = std::function<void(const ContactEvent3D&)>;

} // namespace vshade::physics
