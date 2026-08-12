#include "visual/renderfixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/transform.hpp>
#include <renderer/buffer.hpp>
#include <renderer/camera.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer3d.hpp>
#include <renderer/vertexarray.hpp>

#include <array>
#include <memory>
#include <stdexcept>

namespace {

[[nodiscard]] vshade::renderer::Mesh createTriangleMesh() {
    constexpr std::array<vshade::renderer::MeshVertex, 3> vertices{{
        {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.0F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};

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

TEST_CASE("Renderer3D submits meshes and reports statistics", "[renderer3d]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::renderer::Framebuffer framebuffer(64, 64);
    vshade::tests::visual::beginOffscreenFrame(framebuffer, {0.0F, 0.0F, 0.0F, 1.0F});
    const vshade::renderer::Mesh mesh = createTriangleMesh();

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 10.0F);
    camera.lookAt(
        {0.0F, 0.0F, 2.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform{},
        mesh,
        vshade::renderer::Material{}
    );
    vshade::renderer::Renderer3D::endScene();

    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 1);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 1);
    CHECK_THROWS_AS(
        vshade::renderer::Renderer3D::drawMesh(
            vshade::math::Transform{},
            mesh,
            vshade::renderer::Material{}
        ),
        std::logic_error
    );
}

TEST_CASE("Renderer3D validates directional lights", "[renderer3d]") {
    CHECK_THROWS_AS(
        vshade::renderer::Renderer3D::setDirectionalLight({
            .direction = {0.0F, 0.0F, 0.0F},
        }),
        std::invalid_argument
    );
}
