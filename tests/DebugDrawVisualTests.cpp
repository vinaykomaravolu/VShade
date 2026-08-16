#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <renderer/Camera.hpp>
#include <renderer/DebugDraw.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>

#include <filesystem>

TEST_CASE("DebugDraw primitives match their golden image", "[debug-draw][visual][opengl]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.018F, 0.025F, 0.045F, 1.0F}
    );

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {4.5F, 3.4F, 5.2F},
        {0.0F, 0.1F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::DebugDraw::line(
        {0.0F, 0.0F, 0.0F},
        {2.2F, 0.0F, 0.0F},
        {1.0F, 0.15F, 0.12F, 1.0F}
    );
    vshade::renderer::DebugDraw::line(
        {0.0F, 0.0F, 0.0F},
        {0.0F, 2.2F, 0.0F},
        {0.15F, 1.0F, 0.25F, 1.0F}
    );
    vshade::renderer::DebugDraw::line(
        {0.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 2.2F},
        {0.18F, 0.45F, 1.0F, 1.0F}
    );
    vshade::renderer::DebugDraw::box(
        {
            .minimum = {-1.6F, -0.8F, -0.7F},
            .maximum = {-0.2F, 0.9F, 0.7F},
        },
        {1.0F, 0.72F, 0.12F, 1.0F}
    );
    vshade::renderer::DebugDraw::sphere(
        {1.0F, 0.15F, 0.0F},
        0.9F,
        {0.15F, 0.9F, 1.0F, 1.0F},
        32
    );
    vshade::renderer::DebugDraw::flush(camera);

    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().vertexCount == 222);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 0);

    const auto actual = vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "debug-draw-primitives",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render",
        std::filesystem::path(VSHADE_RENDER_OUTPUT_DIR)
    );
}
