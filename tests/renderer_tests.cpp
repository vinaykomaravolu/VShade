#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <renderer/buffer.hpp>
#include <renderer/camera.hpp>
#include <renderer/mesh.hpp>
#include <renderer/renderer.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>

TEST_CASE("Renderer data types report their byte sizes and components", "[renderer]") {
    using vshade::renderer::ShaderDataType;

    CHECK(vshade::renderer::shaderDataTypeSize(ShaderDataType::Float) == sizeof(float));
    CHECK(vshade::renderer::shaderDataTypeSize(ShaderDataType::Float3) == sizeof(float) * 3);
    CHECK(vshade::renderer::shaderDataTypeSize(ShaderDataType::Int4) == sizeof(int) * 4);

    CHECK(vshade::renderer::shaderDataTypeComponentCount(ShaderDataType::Float2) == 2);
    CHECK(vshade::renderer::shaderDataTypeComponentCount(ShaderDataType::Float4) == 4);
    CHECK(vshade::renderer::shaderDataTypeComponentCount(ShaderDataType::Int3) == 3);
}

TEST_CASE("Buffer layouts calculate offsets and stride", "[renderer]") {
    const vshade::renderer::BufferLayout layout{
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
        {"entityId", vshade::renderer::ShaderDataType::Int},
    };

    REQUIRE(layout.elements().size() == 3);
    CHECK(layout.elements()[0].offset == 0);
    CHECK(layout.elements()[1].offset == sizeof(float) * 3);
    CHECK(layout.elements()[2].offset == sizeof(float) * 5);
    CHECK(layout.stride() == sizeof(float) * 5 + sizeof(int));
    CHECK_FALSE(layout.empty());
}

TEST_CASE("Camera combines projection and view matrices", "[renderer]") {
    vshade::renderer::Camera camera;
    camera.lookAt(
        {0.0F, 0.0F, 5.0F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    const vshade::math::Vec4 transformed =
        camera.viewProjection() * vshade::math::Vec4{0.0F, 0.0F, 0.0F, 1.0F};

    CHECK(transformed.x == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(transformed.y == Catch::Approx(0.0F).margin(0.0001F));
    CHECK(transformed.z == Catch::Approx(-5.0F).margin(0.0001F));
    CHECK(transformed.w == Catch::Approx(1.0F).margin(0.0001F));
}

TEST_CASE("Mesh rejects a missing vertex array", "[renderer]") {
    CHECK_THROWS_AS(
        vshade::renderer::Mesh(std::shared_ptr<vshade::renderer::VertexArray>{}),
        std::invalid_argument
    );
}

TEST_CASE("Renderer starts uninitialized without an OpenGL context", "[renderer]") {
    CHECK_FALSE(vshade::renderer::Renderer::isInitialized());
}
