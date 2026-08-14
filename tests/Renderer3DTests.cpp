#include "visual/RenderFixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/Transform.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/Shader.hpp>
#include <renderer/VertexArray.hpp>

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

TEST_CASE("Renderer3D submits meshes and reports statistics", "[renderer3d][opengl]") {
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

TEST_CASE("Renderer3D submits model node hierarchies", "[renderer3d][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    auto mesh = std::make_shared<vshade::renderer::Mesh>(createTriangleMesh());
    auto material = std::make_shared<vshade::renderer::Material>();
    const vshade::renderer::Model model(
        {{mesh, material}},
        {
            {
                .name = "Root",
                .primitives = {0},
                .children = {1},
            },
            {
                .name = "Child",
                .localTransform = vshade::math::Transform({0.25F, 0.0F, 0.0F}),
                .primitives = {0},
            },
        },
        {0}
    );
    vshade::renderer::Camera camera;

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawModel(vshade::math::Transform{}, model);
    vshade::renderer::Renderer3D::endScene();

    CHECK(vshade::renderer::Renderer3D::stats().meshCount == 2);
    CHECK(vshade::renderer::Renderer3D::stats().drawCalls == 2);
    CHECK_THROWS_AS(
        vshade::renderer::Renderer3D::drawModel(vshade::math::Transform{}, model),
        std::logic_error
    );
}

TEST_CASE("Model rejects cyclic node hierarchies", "[renderer3d]") {
    CHECK_THROWS_AS(
        vshade::renderer::Model(
            {},
            {{.name = "Cycle", .children = {0}}},
            {0}
        ),
        std::invalid_argument
    );
}

TEST_CASE("Renderer3D validates directional lights", "[renderer3d]") {
    CHECK_THROWS_AS(
        vshade::renderer::Renderer3D::setDirectionalLight({
            .direction = {0.0F, 0.0F, 0.0F},
        }),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        vshade::renderer::Renderer3D::setDirectionalLight({
            .color = {-1.0F, 1.0F, 1.0F},
        }),
        std::invalid_argument
    );
}

TEST_CASE("Renderer3D reuses resources and restores pipeline state", "[renderer3d][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    const vshade::renderer::Mesh mesh = createTriangleMesh();
    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 10.0F);
    camera.lookAt({0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});

    vshade::renderer::Renderer::setBlending(true);
    vshade::renderer::Renderer::setDepthTesting(false);
    const auto originalState = vshade::renderer::Renderer::pipelineState();

    for (int frame = 0; frame < 2; ++frame) {
        vshade::renderer::Renderer3D::beginScene(camera);
        vshade::renderer::Renderer3D::drawMesh(
            vshade::math::Transform{},
            mesh,
            vshade::renderer::Material{}
        );
        vshade::renderer::Renderer3D::endScene();
        CHECK(vshade::renderer::Renderer::pipelineState() == originalState);
        CHECK(vshade::renderer::Renderer3D::stats().resourceInitializations == 1);
    }
}

TEST_CASE("Renderer3D rejects shader overrides without its matrix contract", "[renderer3d][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    const vshade::renderer::Mesh mesh = createTriangleMesh();
    auto shader = std::make_shared<vshade::renderer::Shader>(
        "missing-projection",
        R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
void main() { gl_Position = view * model * vec4(aPos, 1.0); }
)glsl",
        R"glsl(#version 330 core
out vec4 fragmentColor;
void main() { fragmentColor = vec4(1.0); }
)glsl"
    );
    vshade::renderer::Material material;
    material.setShader(std::move(shader));
    vshade::renderer::Camera camera;

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(vshade::math::Transform{}, mesh, material);
    const auto sceneState = vshade::renderer::Renderer::pipelineState();
    CHECK_THROWS_AS(vshade::renderer::Renderer3D::endScene(), std::invalid_argument);
    CHECK(vshade::renderer::Renderer::pipelineState() != sceneState);
}

TEST_CASE("Renderer3D snapshots custom material and draw parameters", "[renderer3d][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::renderer::Framebuffer framebuffer(64, 64);
    vshade::tests::visual::beginOffscreenFrame(framebuffer, {0.0F, 0.0F, 0.0F, 1.0F});
    const vshade::renderer::Mesh mesh = createTriangleMesh();
    auto shader = std::make_shared<vshade::renderer::Shader>(
        "custom-parameters",
        R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() { gl_Position = projection * view * model * vec4(aPos, 1.0); }
)glsl",
        R"glsl(#version 330 core
uniform vec4 customColor;
out vec4 fragmentColor;
void main() { fragmentColor = customColor; }
)glsl"
    );

    vshade::renderer::Material material;
    material.setShader(std::move(shader));
    material.parameters().set("customColor", vshade::math::Vec4{1.0F, 0.0F, 0.0F, 1.0F});
    vshade::renderer::DrawParameters drawParameters;
    drawParameters.set("customColor", vshade::math::Vec4{0.2F, 0.8F, 0.4F, 1.0F});

    vshade::renderer::Camera camera;
    camera.setPerspective(0.785398163F, 1.0F, 0.1F, 10.0F);
    camera.lookAt({0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});

    vshade::renderer::Renderer3D::beginScene(camera);
    vshade::renderer::Renderer3D::drawMesh(
        vshade::math::Transform{},
        mesh,
        material,
        drawParameters
    );
    material.parameters().set("customColor", vshade::math::Vec4{0.0F, 0.0F, 1.0F, 1.0F});
    drawParameters.set("customColor", vshade::math::Vec4{1.0F, 1.0F, 0.0F, 1.0F});
    vshade::renderer::Renderer3D::endScene();

    const auto image = vshade::tests::visual::captureFramebuffer(framebuffer);
    const std::size_t center = (32U * image.width + 32U) * 4U;
    REQUIRE(center + 3U < image.pixels.size());
    CHECK(image.pixels[center] == 51U);
    CHECK(image.pixels[center + 1U] == 204U);
    CHECK(image.pixels[center + 2U] == 102U);
    CHECK(image.pixels[center + 3U] == 255U);
}
