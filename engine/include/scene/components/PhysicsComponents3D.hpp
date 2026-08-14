#pragma once

#include "physics/physics3d/PhysicsBody3D.hpp"
#include "physics/physics3d/PhysicsMaterial3D.hpp"
#include "physics/physics3d/PhysicsShape3D.hpp"

namespace vshade::scene {

/** @brief Serializable Jolt rigid-body configuration for a scene entity. */
struct RigidBody3DComponent {
    physics::PhysicsBody3DSettings settings;
};

/** @brief Serializable Jolt collider configuration for a scene entity. */
struct Collider3DComponent {
    physics::PhysicsShape3D shape{physics::BoxShape3D{}};
    physics::PhysicsMaterial3D material;
    math::Vec3 offset{0.0F};
    bool sensor = false;
};

} // namespace vshade::scene
