#pragma once

#include <math/Transform.hpp>
#include <physics/PhysicsTypes.hpp>
#include <physics/physics3d/PhysicsMaterial3D.hpp>
#include <physics/physics3d/PhysicsShape3D.hpp>
#include <physics/physics3d/PhysicsWorld3D.hpp>

namespace vshade::examples::direct3d {

/** Small game-owned adapter that keeps one dynamic helmet body synchronized. */
class HelmetBody final {
public:
    HelmetBody(physics::PhysicsWorld3D& world, const math::Vec3& spawnPosition)
        : m_world(&world),
          m_spawnPosition(spawnPosition),
          m_body(world.createBody(
              physics::PhysicsBody3DSettings{
                  .type = physics::BodyType::Dynamic,
                  .position = spawnPosition,
                  .angularVelocity = {0.4F, 0.7F, 0.2F},
                  .mass = 1.0F,
                  .continuousCollision = true,
              },
              physics::SphereShape3D{0.65F},
              physics::PhysicsMaterial3D{
                  .friction = 0.65F,
                  .restitution = 0.25F,
              }
          )) {}

    [[nodiscard]] physics::PhysicsBody3D body() const noexcept {
        return m_body;
    }

    void synchronize(math::Transform& transform) const {
        transform.setPosition(m_world->position(m_body));
        transform.setRotation(m_world->rotation(m_body));
    }

    void reset() {
        m_world->setTransform(m_body, m_spawnPosition, math::identity());
        m_world->setLinearVelocity(m_body, {0.0F, 0.0F, 0.0F});
    }

private:
    physics::PhysicsWorld3D* m_world = nullptr;
    math::Vec3 m_spawnPosition{0.0F};
    physics::PhysicsBody3D m_body;
};

} // namespace vshade::examples::direct3d
