#include "visual/RenderFixture.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <physics/PhysicsTypes.hpp>
#include <physics/physics2d/PhysicsShape2D.hpp>
#include <physics/physics2d/PhysicsSystem2D.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer2D.hpp>
#include <renderer/Texture.hpp>
#include <scene/Components.hpp>
#include <scene/Entity.hpp>
#include <scene/Scene.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace {

constexpr float fixedTimeStep = 1.0F / 60.0F;

[[nodiscard]] std::shared_ptr<vshade::renderer::Texture2D> createCircleTexture() {
    constexpr std::uint32_t size = 64;
    std::vector<std::uint8_t> pixels(size * size * 4, 0);
    for (std::uint32_t y = 0; y < size; ++y) {
        for (std::uint32_t x = 0; x < size; ++x) {
            const float centeredX = static_cast<float>(x) + 0.5F - size * 0.5F;
            const float centeredY = static_cast<float>(y) + 0.5F - size * 0.5F;
            const bool inside = centeredX * centeredX + centeredY * centeredY <=
                (size * 0.5F - 1.0F) * (size * 0.5F - 1.0F);
            if (!inside) {
                continue;
            }
            const std::size_t pixel = (static_cast<std::size_t>(y) * size + x) * 4;
            pixels[pixel] = 70;
            pixels[pixel + 1] = 165;
            pixels[pixel + 2] = 245;
            pixels[pixel + 3] = 255;
        }
    }
    return std::make_shared<vshade::renderer::Texture2D>(
        size,
        size,
        vshade::renderer::TextureFormat::RGBA8,
        pixels.data(),
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::ClampToEdge
    );
}

void checkBallAndFloorGolden(
    const vshade::math::Vec3& ballPosition,
    const vshade::math::Vec3& floorPosition,
    const std::string_view imageName
) {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    vshade::renderer::Camera camera;
    camera.setOrthographic(-4.0F, 4.0F, -4.0F, 4.0F, -1.0F, 1.0F);
    const auto circleTexture = createCircleTexture();

    vshade::renderer::Renderer2D::beginScene(camera);
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform(
            floorPosition,
            vshade::math::Quat{1.0F, 0.0F, 0.0F, 0.0F},
            {6.0F, 0.5F, 1.0F}
        ),
        {0.32F, 0.37F, 0.45F, 1.0F}
    );
    vshade::renderer::Renderer2D::drawSprite(
        vshade::math::Transform(
            ballPosition,
            vshade::math::Quat{1.0F, 0.0F, 0.0F, 0.0F},
            {1.0F, 1.0F, 1.0F}
        ),
        {
            .texture = circleTexture,
            .color = {1.0F, 1.0F, 1.0F, 1.0F},
            .sortingLayer = 1,
        }
    );
    vshade::renderer::Renderer2D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer2D::stats().quadCount == 2);
    vshade::tests::visual::checkGoldenImage(
        actual,
        imageName,
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "physics2d",
        VSHADE_PHYSICS2D_OUTPUT_DIR
    );
}

} // namespace

TEST_CASE(
    "PhysicsWorld2D ball and floor match their golden image",
    "[physics][physics2d][world][visual][opengl]"
) {
    vshade::physics::PhysicsWorld2D world;
    const auto floor = world.createBody(
        {
            .type = vshade::physics::BodyType::Static,
            .position = {0.0F, -1.3F},
        },
        vshade::physics::BoxShape2D{{3.0F, 0.25F}}
    );
    const auto ball = world.createBody(
        {
            .type = vshade::physics::BodyType::Dynamic,
            .position = {0.0F, 2.5F},
        },
        vshade::physics::CircleShape2D{0.5F}
    );
    for (int step = 0; step < 180; ++step) {
        world.step(fixedTimeStep);
    }

    const vshade::math::Vec2 ballPosition = world.position(ball);
    const vshade::math::Vec2 floorPosition = world.position(floor);
    CHECK(ballPosition.y == Catch::Approx(-0.55F).margin(0.03F));
    checkBallAndFloorGolden(
        {ballPosition.x, ballPosition.y, 0.0F},
        {floorPosition.x, floorPosition.y, 0.0F},
        "physics_world2d_ball_and_floor"
    );
}

TEST_CASE(
    "PhysicsSystem2D ball and floor match their golden image",
    "[physics][physics2d][system][scene][visual][opengl]"
) {
    vshade::scene::Scene scene("Physics2D visual scene");

    vshade::scene::Entity floor = scene.createEntity("Floor");
    floor.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        -1.3F,
        0.0F
    });
    floor.addComponent<vshade::scene::RigidBody2DComponent>().settings.type =
        vshade::physics::BodyType::Static;
    floor.addComponent<vshade::scene::Collider2DComponent>(
        vshade::scene::Collider2DComponent{
            .shape = vshade::physics::BoxShape2D{{3.0F, 0.25F}},
        }
    );

    vshade::scene::Entity ball = scene.createEntity("Ball");
    ball.component<vshade::scene::TransformComponent>().transform.setPosition({
        0.0F,
        2.5F,
        0.0F
    });
    ball.addComponent<vshade::scene::RigidBody2DComponent>().settings.type =
        vshade::physics::BodyType::Dynamic;
    ball.addComponent<vshade::scene::Collider2DComponent>(
        vshade::scene::Collider2DComponent{
            .shape = vshade::physics::CircleShape2D{0.5F},
        }
    );

    vshade::physics::PhysicsSystem2D physics;
    physics.rebuild(scene);
    for (int step = 0; step < 180; ++step) {
        physics.update(scene, fixedTimeStep);
    }

    const vshade::math::Transform& ballPose =
        ball.component<vshade::scene::TransformComponent>().transform;
    CHECK(ballPose.position().y == Catch::Approx(-0.55F).margin(0.03F));

    checkBallAndFloorGolden(
        ballPose.position(),
        floor.component<vshade::scene::TransformComponent>().transform.position(),
        "physics_system2d_ball_and_floor"
    );
}
