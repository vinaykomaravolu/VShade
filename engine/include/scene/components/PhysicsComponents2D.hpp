#pragma once

#include "physics/physics2d/PhysicsBody2D.hpp"
#include "physics/physics2d/PhysicsMaterial2D.hpp"
#include "physics/physics2d/PhysicsShape2D.hpp"

namespace vshade::scene {

/** @brief Serializable Box2D rigid-body configuration for a scene entity. */
struct RigidBody2DComponent {
    physics::PhysicsBody2DSettings settings;
};

/** @brief Serializable Box2D collider configuration for a scene entity. */
struct Collider2DComponent {
    physics::PhysicsShape2D shape{physics::BoxShape2D{}};
    physics::PhysicsMaterial2D material;
    math::Vec2 offset{0.0F};
    bool sensor = false;
};

} // namespace vshade::scene
