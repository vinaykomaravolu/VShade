#pragma once

namespace vshade::physics {

/** @brief Surface properties applied to a three-dimensional collider. */
struct PhysicsMaterial3D {
    float friction = 0.5F;
    float restitution = 0.0F;

    bool operator==(const PhysicsMaterial3D&) const = default;
};

} // namespace vshade::physics
