#pragma once

#include "math/Vector.hpp"
#include "physics/physics2d/Collision2D.hpp"
#include "physics/physics2d/PhysicsBody2D.hpp"
#include "physics/physics2d/PhysicsMaterial2D.hpp"
#include "physics/physics2d/PhysicsShape2D.hpp"

#include <memory>
#include <optional>

namespace vshade::physics {

/** @brief Configuration used to initialize a Box2D simulation. */
struct PhysicsWorld2DSettings {
    math::Vec2 gravity{0.0F, -9.81F};
};

/** @brief Owns one isolated Box2D simulation and all bodies created in it. */
class PhysicsWorld2D final {
public:
    explicit PhysicsWorld2D(const PhysicsWorld2DSettings& settings = {});
    ~PhysicsWorld2D();

    PhysicsWorld2D(PhysicsWorld2D&&) noexcept;
    PhysicsWorld2D& operator=(PhysicsWorld2D&&) noexcept;
    PhysicsWorld2D(const PhysicsWorld2D&) = delete;
    PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;

    [[nodiscard]] PhysicsBody2D createBody(
        const PhysicsBody2DSettings& settings,
        const PhysicsShape2D& shape,
        const PhysicsMaterial2D& material = {},
        const math::Vec2& colliderOffset = math::Vec2{0.0F},
        bool sensor = false
    );
    void destroyBody(PhysicsBody2D body);
    [[nodiscard]] bool contains(PhysicsBody2D body) const noexcept;

    void step(float fixedDeltaTime);
    void setGravity(const math::Vec2& gravity);
    [[nodiscard]] math::Vec2 gravity() const;

    void setTransform(PhysicsBody2D body, const math::Vec2& position, float rotationRadians);
    [[nodiscard]] math::Vec2 position(PhysicsBody2D body) const;
    [[nodiscard]] float rotation(PhysicsBody2D body) const;
    void setLinearVelocity(PhysicsBody2D body, const math::Vec2& velocity);
    [[nodiscard]] math::Vec2 linearVelocity(PhysicsBody2D body) const;
    void applyForce(PhysicsBody2D body, const math::Vec2& force);
    void applyImpulse(PhysicsBody2D body, const math::Vec2& impulse);

    [[nodiscard]] std::optional<RaycastHit2D> raycast(const RaycastQuery2D& query) const;
    void setContactListener(ContactListener2D listener);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::physics
