#include "visual/renderfixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/quaternion.hpp>
#include <math/transform.hpp>
#include <renderer/camera.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer2d.hpp>
#include <renderer/texture.hpp>

#include <array>
#include <cstdint>
#include <memory>

TEST_CASE("Renderer2D scene matches its golden image", "[renderer2d][visual]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.035F, 0.045F, 0.075F, 1.0F}
    );

    constexpr std::array<std::uint8_t, 16> checkerPixels{
        250, 195, 45, 255,
        45, 80, 190, 255,
        45, 80, 190, 255,
        250, 195, 45, 255,
    };
    auto checkerTexture = std::make_shared<vshade::renderer::Texture2D>(
        2,
        2,
        vshade::renderer::TextureFormat::RGBA8,
        checkerPixels.data(),
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::Repeat
    );

    vshade::renderer::Camera camera;
    camera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);

    vshade::renderer::Renderer2D::beginScene(camera);

    // Submitted out of order intentionally; sortingLayer determines the result.
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform(
            {0.25F, 0.08F, 0.0F},
            vshade::math::fromEuler({0.0F, 0.0F, -0.18F}),
            {0.95F, 0.72F, 1.0F}
        ),
        {0.95F, 0.20F, 0.18F, 0.58F},
        2
    );
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform(
            {-0.28F, 0.18F, 0.0F},
            vshade::math::fromEuler({0.0F, 0.0F, 0.12F}),
            {1.15F, 1.05F, 1.0F}
        ),
        {0.10F, 0.32F, 0.78F, 1.0F},
        -1
    );
    vshade::renderer::Renderer2D::drawSprite(
        vshade::math::Transform(
            {-0.22F, -0.20F, 0.0F},
            vshade::math::fromEuler({0.0F, 0.0F, 0.28F}),
            {0.72F, 0.72F, 1.0F}
        ),
        {
            .texture = checkerTexture,
            .color = {1.0F, 1.0F, 1.0F, 0.92F},
            .tiling = {2.0F, 2.0F},
            .sortingLayer = 1,
        }
    );

    vshade::renderer::Renderer2D::endScene();
    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);

    CHECK(vshade::renderer::Renderer2D::stats().quadCount == 3);
    CHECK(vshade::renderer::Renderer2D::stats().drawCalls == 3);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "renderer2d_scene",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render2d",
        VSHADE_RENDER2D_OUTPUT_DIR
    );
}
