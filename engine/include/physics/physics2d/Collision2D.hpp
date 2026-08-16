#pragma once

#include "math/Vector.hpp"
#include "physics/PhysicsTypes.hpp"

#include <functional>

namespace vshade::physics {

/** @brief Result returned by a successful two-dimensional ray cast. */
struct RaycastHit2D {
    BodyId2D body;
    math::Vec2 point{0.0F};
    math::Vec2 normal{0.0F};
    float fraction = 0.0F;
};

/** @brief Ray query expressed as an origin and finite displacement. */
struct RaycastQuery2D {
    math::Vec2 origin{0.0F};
    math::Vec2 displacement{0.0F};
    CollisionFilter filter;
};

/** @brief Engine-facing contact notification from Box2D. */
struct ContactEvent2D {
    BodyId2D firstBody;
    BodyId2D secondBody;
    math::Vec2 normal{0.0F};
    math::Vec2 point{0.0F};
    ContactPhase phase = ContactPhase::Began;
};

using ContactListener2D = std::function<void(const ContactEvent2D&)>;

} // namespace vshade::physics
