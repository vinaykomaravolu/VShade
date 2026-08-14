#include <physics/physics2d/PhysicsSystem2D.hpp>
#include <physics/physics2d/PhysicsWorld2D.hpp>
#include <scene/Components.hpp>
#include <scene/Entity.hpp>
#include <scene/Scene.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {

using vshade::physics::BodyType;
using vshade::physics::BoxShape2D;
using vshade::physics::CircleShape2D;
using vshade::physics::ContactEvent2D;
using vshade::physics::ContactPhase;
using vshade::physics::PhysicsBody2DSettings;
using vshade::physics::PhysicsMaterial2D;
using vshade::physics::PhysicsWorld2D;

constexpr float fixedTimeStep = 1.0F / 60.0F;

[[nodiscard]] PhysicsBody2DSettings dynamicBodyAt(const float x, const float y) {
    return {
        .type = BodyType::Dynamic,
        .position = {x, y},
    };
}

} // namespace

TEST_CASE("Physics2D simulates gravity and solid collisions", "[physics][physics2d]") {
    PhysicsWorld2D world;
    CHECK(world.gravity().x == Catch::Approx(0.0F));
    CHECK(world.gravity().y == Catch::Approx(-9.81F));

    const auto floor = world.createBody(
        {.type = BodyType::Static, .position = {0.0F, 0.0F}},
        BoxShape2D{{5.0F, 0.5F}}
    );
    const auto ball = world.createBody(
        dynamicBodyAt(0.0F, 3.0F),
        CircleShape2D{0.5F},
        PhysicsMaterial2D{.density = 1.0F, .friction = 0.4F}
    );

    for (int step = 0; step < 180; ++step) {
        world.step(fixedTimeStep);
    }

    CHECK(world.position(floor).y == Catch::Approx(0.0F));
    CHECK(world.position(ball).y == Catch::Approx(1.0F).margin(0.03F));
    CHECK(world.linearVelocity(ball).y == Catch::Approx(0.0F).margin(0.05F));
}

TEST_CASE("Physics2D handles impulses raycasts and stale handles", "[physics][physics2d]") {
    PhysicsWorld2D world({.gravity = {0.0F, 0.0F}});
    const auto body = world.createBody(
        dynamicBodyAt(0.0F, 0.0F),
        CircleShape2D{0.5F}
    );
    PhysicsWorld2D foreignWorld({.gravity = {0.0F, 0.0F}});
    CHECK_FALSE(foreignWorld.contains(body));
    CHECK_THROWS_AS(foreignWorld.position(body), std::invalid_argument);

    world.applyImpulse(body, {2.0F, 0.0F});
    CHECK(world.linearVelocity(body).x > 0.0F);

    const auto hit = world.raycast({
        .origin = {-2.0F, 0.0F},
        .displacement = {4.0F, 0.0F},
    });
    REQUIRE(hit.has_value());
    CHECK(hit->body == body.id());
    CHECK(hit->point.x == Catch::Approx(-0.5F).margin(0.01F));
    CHECK(hit->normal.x == Catch::Approx(-1.0F).margin(0.01F));

    const auto stale = body;
    world.destroyBody(body);
    CHECK_FALSE(world.contains(stale));
    CHECK_THROWS_AS(world.position(stale), std::invalid_argument);

    const auto replacement = world.createBody(
        dynamicBodyAt(0.0F, 0.0F),
        CircleShape2D{0.5F}
    );
    CHECK(replacement.id() != stale.id());
    CHECK(world.contains(replacement));
}

TEST_CASE("Physics2D reports contact phases after stepping", "[physics][physics2d]") {
    PhysicsWorld2D world;
    const auto floor = world.createBody(
        {.type = BodyType::Static},
        BoxShape2D{{3.0F, 0.5F}}
    );
    const auto ball = world.createBody(
        dynamicBodyAt(0.0F, 2.0F),
        CircleShape2D{0.5F}
    );

    std::vector<ContactEvent2D> events;
    world.setContactListener([&events](const ContactEvent2D& event) {
        events.push_back(event);
    });

    for (int step = 0; step < 120; ++step) {
        world.step(fixedTimeStep);
    }

    const auto involvesBodies = [floor, ball](const ContactEvent2D& event) {
        return (event.firstBody == floor.id() && event.secondBody == ball.id()) ||
            (event.firstBody == ball.id() && event.secondBody == floor.id());
    };
    CHECK(std::ranges::any_of(events, [involvesBodies](const ContactEvent2D& event) {
        return event.phase == ContactPhase::Began && involvesBodies(event);
    }));
    CHECK(std::ranges::any_of(events, [involvesBodies](const ContactEvent2D& event) {
        return event.phase == ContactPhase::Persisted && involvesBodies(event);
    }));
}

TEST_CASE("Physics2D validates unsafe public inputs", "[physics][physics2d]") {
    PhysicsWorld2D world;
    CHECK_THROWS_AS(world.step(0.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        world.createBody({}, BoxShape2D{{0.0F, 1.0F}}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        world.createBody(
            {},
            CircleShape2D{0.5F},
            PhysicsMaterial2D{.density = -1.0F}
        ),
        std::invalid_argument
    );
    CHECK_FALSE(world.raycast({
        .origin = {0.0F, 0.0F},
        .displacement = {0.0F, 0.0F},
    }).has_value());
}

TEST_CASE("PhysicsSystem2D runs a simple platformer scene", "[physics][physics2d][scene][example]") {
    vshade::scene::Scene scene("Simple 2D platformer");

    // A Scene entity participates in physics when it has a rigid body and a
    // collider. Every newly created entity already has a TransformComponent.
    vshade::scene::Entity floor = scene.createEntity("Floor");
    floor.addComponent<vshade::scene::RigidBody2DComponent>().settings.type =
        BodyType::Static;
    floor.addComponent<vshade::scene::Collider2DComponent>(
        vshade::scene::Collider2DComponent{
            .shape = BoxShape2D{{5.0F, 0.5F}},
        }
    );

    vshade::scene::Entity player = scene.createEntity("Player");
    player.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        3.0F,
        7.0F
    });
    player.addComponent<vshade::scene::RigidBody2DComponent>().settings.type =
        BodyType::Dynamic;
    player.addComponent<vshade::scene::Collider2DComponent>(
        vshade::scene::Collider2DComponent{
            .shape = CircleShape2D{0.5F},
        }
    );

    // Build runtime Box2D bodies after loading or constructing the Scene.
    vshade::physics::PhysicsSystem2D physics;
    physics.rebuild(scene);

    // A game calls update from its fixed-update loop. Dynamic body transforms
    // are copied back to their Scene entities after each Box2D step.
    for (int step = 0; step < 180; ++step) {
        physics.update(scene, fixedTimeStep);
    }

    const auto& transform =
        player.component<vshade::scene::TransformComponent>().transform;
    CHECK(transform.position().y == Catch::Approx(1.0F).margin(0.03F));
    CHECK(transform.position().z == Catch::Approx(7.0F));

    // Removing a required physics component causes update() to remove the
    // corresponding runtime body safely.
    player.removeComponent<vshade::scene::Collider2DComponent>();
    CHECK_NOTHROW(physics.update(scene, fixedTimeStep));
}
