#include "assets/Bounce.hpp"
#include "assets/RotatePlatform.hpp"
#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/Transform.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer2D.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <script/Scripting.hpp>

#include <filesystem>

TEST_CASE("Native user scripts produce the expected visual state", "[scripting][visual][opengl]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );

    vshade::script::NativeScriptRegistry registry;
    registry.registerType<vshade::tests::assets::RotatePlatform>("RotatePlatform");
    registry.registerType<vshade::tests::assets::Bounce>("Bounce");

    vshade::scene::Scene scene("Native script visual");
    vshade::scene::Entity platform = scene.createEntity("Rotating platform");
    auto& platformTransform =
        platform.component<vshade::scene::TransformComponent>().transform;
    platformTransform.setPosition({-0.2F, -0.3F, 0.0F});
    platformTransform.setScale({1.15F, 0.24F, 1.0F});
    platform.addComponent<vshade::scene::ScriptComponent>(
        vshade::scene::ScriptComponent{{{.typeName = "RotatePlatform"}}}
    );

    vshade::scene::Entity ball = scene.createEntity("Bouncing object");
    auto& ballTransform = ball.component<vshade::scene::TransformComponent>().transform;
    ballTransform.setPosition({0.38F, -0.22F, 0.0F});
    ballTransform.setScale({0.28F, 0.28F, 1.0F});
    ball.addComponent<vshade::scene::ScriptComponent>(
        vshade::scene::ScriptComponent{{{.typeName = "Bounce"}}}
    );

    vshade::script::NativeScriptSystem scripts(registry);
    scripts.attachScene(scene);
    scripts.update(0.55F);
    CHECK(scripts.instanceCount() == 2);

    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.065F, 1.0F}
    );
    vshade::renderer::Camera camera;
    camera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);
    vshade::renderer::Renderer2D::beginScene(camera);
    vshade::renderer::Renderer2D::drawQuad(
        platformTransform,
        {0.18F, 0.72F, 0.95F, 1.0F},
        0
    );
    vshade::renderer::Renderer2D::drawQuad(
        ballTransform,
        {1.0F, 0.48F, 0.12F, 1.0F},
        1
    );
    vshade::renderer::Renderer2D::endScene();
    scripts.detachScene();

    const auto actual = vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "native_scripting",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render2d",
        std::filesystem::path(VSHADE_RENDER2D_OUTPUT_DIR)
    );
}
