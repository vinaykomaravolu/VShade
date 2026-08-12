#include "visual/renderfixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/transform.hpp>
#include <renderer/camera.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer2d.hpp>

#include <stdexcept>

TEST_CASE("Renderer2D submits colored quads and reports statistics", "[renderer2d]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::renderer::Framebuffer framebuffer(64, 64);
    vshade::tests::visual::beginOffscreenFrame(framebuffer, {0.0F, 0.0F, 0.0F, 1.0F});

    vshade::renderer::Camera camera;
    camera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);

    vshade::renderer::Renderer2D::beginScene(camera);
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform{},
        {0.25F, 0.5F, 0.75F, 1.0F}
    );
    vshade::renderer::Renderer2D::endScene();

    CHECK(vshade::renderer::Renderer2D::stats().quadCount == 1);
    CHECK(vshade::renderer::Renderer2D::stats().drawCalls == 1);
    CHECK_THROWS_AS(
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform{},
            vshade::math::Vec4{1.0F}
        ),
        std::logic_error
    );
}

TEST_CASE("Renderer2D rejects nested scenes and supports an empty scene", "[renderer2d]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::renderer::Camera camera;
    camera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);

    vshade::renderer::Renderer2D::beginScene(camera);
    CHECK_THROWS_AS(vshade::renderer::Renderer2D::beginScene(camera), std::logic_error);
    vshade::renderer::Renderer2D::endScene();

    CHECK(vshade::renderer::Renderer2D::stats().quadCount == 0);
    CHECK(vshade::renderer::Renderer2D::stats().drawCalls == 0);
}
