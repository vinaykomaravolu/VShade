#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <renderer/Camera.hpp>
#include <renderer/DebugDraw.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>

#include <limits>
#include <stdexcept>

TEST_CASE("DebugDraw queues line-based primitives", "[debug-draw]") {
    using vshade::renderer::DebugBounds;
    using vshade::renderer::DebugDraw;

    DebugDraw::clear();
    CHECK(DebugDraw::lineCount() == 0);

    DebugDraw::line(
        {0.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F, 1.0F}
    );
    CHECK(DebugDraw::lineCount() == 1);

    DebugDraw::box(
        DebugBounds{
            .minimum = {-1.0F, -1.0F, -1.0F},
            .maximum = {1.0F, 1.0F, 1.0F},
        },
        {0.0F, 1.0F, 0.0F, 1.0F}
    );
    CHECK(DebugDraw::lineCount() == 13);

    DebugDraw::sphere(
        {0.0F, 0.0F, 0.0F},
        1.0F,
        {0.0F, 0.5F, 1.0F, 1.0F},
        8
    );
    CHECK(DebugDraw::lineCount() == 37);

    DebugDraw::clear();
    CHECK(DebugDraw::lineCount() == 0);
}

TEST_CASE("DebugDraw rejects invalid primitives", "[debug-draw]") {
    using vshade::renderer::DebugBounds;
    using vshade::renderer::DebugDraw;

    DebugDraw::clear();
    CHECK_THROWS_AS(
        DebugDraw::box(
            DebugBounds{
                .minimum = {1.0F, 0.0F, 0.0F},
                .maximum = {-1.0F, 1.0F, 1.0F},
            },
            {1.0F, 1.0F, 1.0F, 1.0F}
        ),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        DebugDraw::sphere(
            {0.0F, 0.0F, 0.0F},
            0.0F,
            {1.0F, 1.0F, 1.0F, 1.0F}
        ),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        DebugDraw::sphere(
            {0.0F, 0.0F, 0.0F},
            1.0F,
            {1.0F, 1.0F, 1.0F, 1.0F},
            2
        ),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        DebugDraw::line(
            {std::numeric_limits<float>::quiet_NaN(), 0.0F, 0.0F},
            {1.0F, 0.0F, 0.0F},
            {1.0F, 1.0F, 1.0F, 1.0F}
        ),
        std::invalid_argument
    );
    CHECK(DebugDraw::lineCount() == 0);
}

TEST_CASE("DebugDraw flushes once and restores pipeline state", "[debug-draw][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::renderer::Framebuffer framebuffer(64, 64);
    vshade::tests::visual::beginOffscreenFrame(framebuffer, {0.0F, 0.0F, 0.0F, 1.0F});

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 10.0F);
    camera.lookAt(
        {0.0F, 0.0F, 3.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );
    vshade::renderer::Renderer::setBlending(false);
    vshade::renderer::Renderer::setDepthWrite(true);
    const auto initialState = vshade::renderer::Renderer::pipelineState();

    vshade::renderer::DebugDraw::line(
        {-0.5F, 0.0F, 0.0F},
        {0.5F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F, 1.0F}
    );
    vshade::renderer::DebugDraw::flush(camera);

    CHECK(vshade::renderer::DebugDraw::lineCount() == 0);
    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().vertexCount == 2);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 0);
    CHECK(vshade::renderer::Renderer::pipelineState() == initialState);

    vshade::renderer::Framebuffer::unbind();
}
