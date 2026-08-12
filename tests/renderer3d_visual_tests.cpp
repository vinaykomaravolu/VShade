#include "visual/renderfixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/quaternion.hpp>
#include <math/transform.hpp>
#include <renderer/buffer.hpp>
#include <renderer/camera.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer2d.hpp>
#include <renderer/renderer3d.hpp>
#include <renderer/texture.hpp>
#include <renderer/vertexarray.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace {

[[nodiscard]] vshade::renderer::Mesh createCubeMesh() {
    using vshade::renderer::MeshVertex;
    constexpr std::array<MeshVertex, 24> vertices{{
        // Front (+Z)
        {{-0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        // Back (-Z)
        {{0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F}},
        // Left (-X)
        {{-0.7F, -0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Right (+X)
        {{0.7F, -0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Top (+Y)
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
        // Bottom (-Y)
        {{-0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 36> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"normal", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
    });
    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    auto vertexArray = std::make_shared<vshade::renderer::VertexArray>();
    vertexArray->addVertexBuffer(std::move(vertexBuffer));
    vertexArray->setIndexBuffer(std::move(indexBuffer));
    return vshade::renderer::Mesh(std::move(vertexArray));
}

} // namespace

TEST_CASE("Renderer3D lit mesh matches its golden image", "[renderer3d][visual]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    const vshade::renderer::Mesh cube = createCubeMesh();
    constexpr std::array<std::uint8_t, 16> texturePixels{
        235, 225, 170, 255,
        55, 135, 105, 255,
        55, 135, 105, 255,
        235, 225, 170, 255,
    };
    auto texture = std::make_shared<vshade::renderer::Texture2D>(
        2,
        2,
        vshade::renderer::TextureFormat::RGBA8,
        texturePixels.data(),
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::ClampToEdge
    );

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    camera.lookAt(
        {3.2F, 2.4F, 4.2F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::Renderer3D::setDirectionalLight({
        .direction = {-0.55F, -1.0F, -0.35F},
        .color = {1.0F, 0.94F, 0.82F},
        .intensity = 0.95F,
    });
    vshade::renderer::Material material(vshade::renderer::MaterialShading::Lit);
    material.setAlbedoTexture(texture);
    material.setAlbedoColor({0.88F, 1.0F, 0.92F, 1.0F});
    // The neutral value preserves this test's established lighting baseline;
    // non-neutral material response is demonstrated by the sandbox.
    material.setRoughness(0.5F);
    material.setMetallic(0.0F);

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::fromEuler({0.38F, 0.62F, 0.10F}),
            {1.0F, 1.0F, 1.0F}
        ),
        cube,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    // Render an overlay between 3D frames. Its temporary index buffer must not
    // replace the cube VAO's index buffer, and its disabled depth writes must
    // not prevent the next frame's depth clear.
    vshade::renderer::Camera overlayCamera;
    overlayCamera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);
    vshade::renderer::Renderer2D::beginScene(overlayCamera);
    vshade::renderer::Renderer2D::drawQuad(
        vshade::math::Transform{},
        {1.0F, 1.0F, 1.0F, 1.0F}
    );
    vshade::renderer::Renderer2D::endScene();

    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );
    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform(
            {0.0F, 0.0F, 0.0F},
            vshade::math::fromEuler({0.38F, 0.62F, 0.10F}),
            {1.0F, 1.0F, 1.0F}
        ),
        cube,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 1);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 1);
    vshade::tests::visual::checkGoldenImage(actual, "renderer3d_lit_cube");
}
