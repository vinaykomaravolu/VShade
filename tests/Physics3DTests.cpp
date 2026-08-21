#include <physics/physics3d/PhysicsSystem3D.hpp>
#include <physics/physics3d/PhysicsWorld3D.hpp>
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
using vshade::physics::BoxShape3D;
using vshade::physics::ContactEvent3D;
using vshade::physics::ContactPhase;
using vshade::physics::PhysicsBody3DSettings;
using vshade::physics::PhysicsMaterial3D;
using vshade::physics::PhysicsWorld3D;
using vshade::physics::SphereShape3D;
using vshade::physics::CylinderShape3D;
using vshade::physics::MeshShape3D;
using vshade::physics::ConvexShape3D;

constexpr float fixedTimeStep = 1.0F / 60.0F;

[[nodiscard]] PhysicsBody3DSettings dynamicBodyAt(
    const float x,
    const float y,
    const float z
) {
    return {
        .type = BodyType::Dynamic,
        .position = {x, y, z},
    };
}

} // namespace

TEST_CASE("Physics3D simulates gravity and solid collisions", "[physics][physics3d]") {
    PhysicsWorld3D world({.maxBodies = 1'024});
    CHECK(world.gravity().x == Catch::Approx(0.0F));
    CHECK(world.gravity().y == Catch::Approx(-9.81F));
    CHECK(world.gravity().z == Catch::Approx(0.0F));

    const auto floor = world.createBody(
        {.type = BodyType::Static, .position = {0.0F, 0.0F, 0.0F}},
        BoxShape3D{{5.0F, 0.5F, 5.0F}}
    );
    const auto ball = world.createBody(
        dynamicBodyAt(0.0F, 3.0F, 0.0F),
        SphereShape3D{0.5F},
        PhysicsMaterial3D{.friction = 0.4F}
    );

    for (int step = 0; step < 180; ++step) {
        world.step(fixedTimeStep);
    }

    CHECK(world.position(floor).y == Catch::Approx(0.0F));
    CHECK(world.position(ball).y == Catch::Approx(1.0F).margin(0.04F));
    CHECK(world.linearVelocity(ball).y == Catch::Approx(0.0F).margin(0.08F));
}

TEST_CASE("Physics3D handles impulses raycasts and stale handles", "[physics][physics3d]") {
    PhysicsWorld3D world({
        .gravity = vshade::math::Vec3{0.0F},
        .maxBodies = 1'024
    });
    const auto body = world.createBody(
        dynamicBodyAt(0.0F, 0.0F, 0.0F),
        SphereShape3D{0.5F}
    );
    PhysicsWorld3D foreignWorld({
        .gravity = vshade::math::Vec3{0.0F},
        .maxBodies = 1'024
    });
    CHECK_FALSE(foreignWorld.contains(body));
    CHECK_THROWS_AS(foreignWorld.position(body), std::invalid_argument);

    world.applyImpulse(body, {2.0F, 0.0F, 0.0F});
    CHECK(world.linearVelocity(body).x > 0.0F);

    const auto hit = world.raycast({
        .origin = {-2.0F, 0.0F, 0.0F},
        .displacement = {4.0F, 0.0F, 0.0F},
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
        dynamicBodyAt(0.0F, 0.0F, 0.0F),
        SphereShape3D{0.5F}
    );
    CHECK(replacement.id() != stale.id());
    CHECK(world.contains(replacement));
}

TEST_CASE("Physics3D creates cylinder mesh and convex shapes", "[physics][physics3d]") {
    PhysicsWorld3D world({.maxBodies = 1'024});
    const auto floor = world.createBody(
        {.type = BodyType::Static},
        MeshShape3D{
            .vertices = {
                {-4.0F, 0.0F, -4.0F},
                { 4.0F, 0.0F, -4.0F},
                { 4.0F, 0.0F,  4.0F},
                {-4.0F, 0.0F,  4.0F},
            },
            .indices = {0, 2, 1, 0, 3, 2},
        }
    );
    const auto cylinder = world.createBody(
        dynamicBodyAt(-1.0F, 3.0F, 0.0F),
        CylinderShape3D{0.5F, 0.5F}
    );
    const auto convex = world.createBody(
        dynamicBodyAt(1.0F, 3.0F, 0.0F),
        ConvexShape3D{{
            {-0.5F, -0.5F, -0.5F},
            { 0.5F, -0.5F, -0.5F},
            { 0.0F, -0.5F,  0.5F},
            { 0.0F,  0.5F,  0.0F},
        }}
    );

    for (int step = 0; step < 180; ++step) {
        world.step(fixedTimeStep);
    }

    CHECK(world.contains(floor));
    CHECK(world.position(cylinder).y == Catch::Approx(0.5F).margin(0.06F));
    CHECK(world.position(convex).y > 0.35F);
    CHECK_THROWS_AS(
        world.createBody(
            dynamicBodyAt(0.0F, 1.0F, 0.0F),
            MeshShape3D{
                .vertices = {
                    {0.0F, 0.0F, 0.0F},
                    {1.0F, 0.0F, 0.0F},
                    {0.0F, 0.0F, 1.0F},
                },
                .indices = {0, 2, 1},
            }
        ),
        std::invalid_argument
    );
}

TEST_CASE("Physics3D reports contact phases after stepping", "[physics][physics3d]") {
    PhysicsWorld3D world({.maxBodies = 1'024});
    const auto floor = world.createBody(
        {.type = BodyType::Static},
        BoxShape3D{{3.0F, 0.5F, 3.0F}}
    );
    const auto ball = world.createBody(
        dynamicBodyAt(0.0F, 2.0F, 0.0F),
        SphereShape3D{0.5F}
    );

    std::vector<ContactEvent3D> events;
    world.setContactListener([&events](const ContactEvent3D& event) {
        events.push_back(event);
    });

    for (int step = 0; step < 120; ++step) {
        world.step(fixedTimeStep);
    }

    const auto involvesBodies = [floor, ball](const ContactEvent3D& event) {
        return (event.firstBody == floor.id() && event.secondBody == ball.id()) ||
            (event.firstBody == ball.id() && event.secondBody == floor.id());
    };
    CHECK(std::ranges::any_of(events, [involvesBodies](const ContactEvent3D& event) {
        return event.phase == ContactPhase::Began && involvesBodies(event);
    }));
    CHECK(std::ranges::any_of(events, [involvesBodies](const ContactEvent3D& event) {
        return event.phase == ContactPhase::Persisted && involvesBodies(event);
    }));
}

TEST_CASE("Physics3D validates unsafe public inputs", "[physics][physics3d]") {
    PhysicsWorld3D world({.maxBodies = 1'024});
    CHECK_THROWS_AS(world.step(0.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        world.createBody({}, BoxShape3D{{0.0F, 1.0F, 1.0F}}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        world.createBody(
            {},
            SphereShape3D{0.5F},
            PhysicsMaterial3D{.friction = -1.0F}
        ),
        std::invalid_argument
    );
    CHECK_FALSE(world.raycast({
        .origin = vshade::math::Vec3{0.0F},
        .displacement = vshade::math::Vec3{0.0F},
    }).has_value());
}

TEST_CASE("PhysicsSystem3D runs a simple falling-ball scene", "[physics][physics3d][scene][example]") {
    vshade::scene::Scene scene("Simple 3D physics scene");

    // Static bodies receive their pose from the Scene and do not move under
    // gravity. The box extends half a unit above and below y = 0.
    vshade::scene::Entity floor = scene.createEntity("Floor");
    floor.addComponent<vshade::scene::RigidBody3DComponent>().settings.type =
        BodyType::Static;
    floor.addComponent<vshade::scene::Collider3DComponent>(
        vshade::scene::Collider3DComponent{
            .shape = BoxShape3D{{5.0F, 0.5F, 5.0F}},
        }
    );

    vshade::scene::Entity ball = scene.createEntity("Ball");
    ball.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        3.0F,
        0.0F
    });
    ball.component<vshade::scene::TransformComponent>().transform.setScale({
        2.0F,
        3.0F,
        4.0F
    });
    ball.addComponent<vshade::scene::RigidBody3DComponent>().settings.type =
        BodyType::Dynamic;
    ball.addComponent<vshade::scene::Collider3DComponent>(
        vshade::scene::Collider3DComponent{
            .shape = SphereShape3D{0.5F},
        }
    );

    // rebuild() converts the Scene component descriptions into runtime Jolt
    // bodies. It is normally called once after a Scene is loaded.
    vshade::physics::PhysicsSystem3D physics({.maxBodies = 1'024});
    physics.rebuild(scene);
    REQUIRE(physics.body(ball).has_value());
    CHECK_NOTHROW(physics.applyImpulse(ball, {0.0F, 0.0F, 0.0F}));
    const auto sceneHit = physics.raycast({
        .origin = {0.0F, 6.0F, 0.0F},
        .displacement = {0.0F, -10.0F, 0.0F},
    });
    REQUIRE(sceneHit.has_value());
    CHECK(sceneHit->entity == ball);
    bool receivedSceneContact = false;
    physics.setContactListener(
        [&](const vshade::physics::SceneContactEvent3D& event) {
            receivedSceneContact =
                (event.first == ball && event.second == floor) ||
                (event.first == floor && event.second == ball);
        }
    );

    // Run physics at a fixed rate. Jolt's dynamic pose is written back into
    // the ball's TransformComponent so the renderer sees the simulated pose.
    for (int step = 0; step < 180; ++step) {
        physics.update(scene, fixedTimeStep);
    }

    const auto& transform =
        ball.component<vshade::scene::TransformComponent>().transform;
    CHECK(transform.position().y == Catch::Approx(1.0F).margin(0.04F));
    CHECK(transform.position().z == Catch::Approx(0.0F));
    CHECK(receivedSceneContact);
    // Physics updates position and rotation, but deliberately preserves the
    // visual scale stored by the Scene.
    CHECK(transform.scale().x == Catch::Approx(2.0F));
    CHECK(transform.scale().y == Catch::Approx(3.0F));
    CHECK(transform.scale().z == Catch::Approx(4.0F));

    ball.removeComponent<vshade::scene::Collider3DComponent>();
    CHECK_NOTHROW(physics.update(scene, fixedTimeStep));
    CHECK_FALSE(physics.body(ball).has_value());
}
