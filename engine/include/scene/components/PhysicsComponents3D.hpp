#pragma once

#include "physics/physics3d/PhysicsBody3D.hpp"
#include "physics/physics3d/PhysicsMaterial3D.hpp"
#include "physics/physics3d/PhysicsShape3D.hpp"

#include "asset/AssetReference.hpp"

namespace vshade::renderer { class Model; }

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

/** Common material, offset, and trigger state shared by typed 3D colliders. */
struct Collider3DProperties {
    physics::PhysicsMaterial3D material;
    math::Vec3 offset{0.0F};
    bool sensor = false;
};

struct BoxCollider3DComponent : Collider3DProperties {
    math::Vec3 halfExtents{0.5F};
};

struct SphereCollider3DComponent : Collider3DProperties {
    float radius = 0.5F;
};

struct CapsuleCollider3DComponent : Collider3DProperties {
    float halfHeight = 0.5F;
    float radius = 0.25F;
};

struct CylinderCollider3DComponent : Collider3DProperties {
    float halfHeight = 0.5F;
    float radius = 0.5F;
};

/** Static triangle collision generated from the referenced model. */
struct MeshCollider3DComponent : Collider3DProperties {
    asset::AssetReference<renderer::Model> model;
};

/** Convex hull collision generated from the referenced model. */
struct ConvexCollider3DComponent : Collider3DProperties {
    asset::AssetReference<renderer::Model> model;
};

} // namespace vshade::scene
