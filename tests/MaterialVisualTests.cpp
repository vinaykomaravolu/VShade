#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/Transform.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Material.hpp>
#include <renderer/Mesh.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/Texture.hpp>
#include <renderer/VertexArray.hpp>

#include <array>
#include <cstdint>
#include <memory>

namespace {

[[nodiscard]] vshade::renderer::Mesh createMaterialQuad() {
    constexpr std::array<vshade::renderer::MeshVertex, 4> vertices{{
        {{-0.75F, -0.60F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.75F, -0.60F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.75F, 0.60F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{-0.75F, 0.60F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 3, 0};

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"normal", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
        {"tangent", vshade::renderer::ShaderDataType::Float4},
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

TEST_CASE("Unlit textured material matches its golden image", "[material][visual][opengl]") {
    vshade::tests::visual::HiddenRenderContext context;
    vshade::renderer::Framebuffer framebuffer(
        vshade::tests::visual::defaultRenderWidth,
        vshade::tests::visual::defaultRenderHeight
    );
    vshade::tests::visual::beginOffscreenFrame(
        framebuffer,
        {0.025F, 0.035F, 0.06F, 1.0F}
    );

    constexpr std::array<std::uint8_t, 16> pixels{
        235, 70, 175, 255,
        45, 210, 220, 255,
        45, 210, 220, 255,
        235, 70, 175, 255,
    };
    auto texture = std::make_shared<vshade::renderer::Texture2D>(
        2,
        2,
        vshade::renderer::TextureFormat::RGBA8,
        pixels.data(),
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::ClampToEdge
    );

    vshade::renderer::Material material(vshade::renderer::MaterialShading::Unlit);
    material.setAlbedoTexture(texture);
    material.setAlbedoColor({0.82F, 0.92F, 1.0F, 1.0F});
    material.setRoughness(0.3F);
    material.setMetallic(0.1F);

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 10.0F);
    camera.lookAt(
        {0.0F, 0.0F, 2.6F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    const vshade::renderer::Mesh quad = createMaterialQuad();
    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform{},
        quad,
        material
    );
    vshade::renderer::Renderer3D::endScene();

    const vshade::tests::visual::Image actual =
        vshade::tests::visual::captureFramebuffer(framebuffer);
    vshade::tests::visual::checkGoldenImage(
        actual,
        "material_unlit_quad",
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "material",
        VSHADE_MATERIAL_OUTPUT_DIR
    );
}
