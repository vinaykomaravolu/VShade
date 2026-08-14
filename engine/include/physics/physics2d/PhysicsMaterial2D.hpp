#pragma once

namespace vshade::physics {

/** @brief Surface and mass properties applied to a two-dimensional collider. */
struct PhysicsMaterial2D {
    float density = 1.0F;
    float friction = 0.5F;
    float restitution = 0.0F;

    bool operator==(const PhysicsMaterial2D&) const = default;
};

} // namespace vshade::physics
