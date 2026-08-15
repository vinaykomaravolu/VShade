#pragma once

#include <vshade/Game.hpp>

#include <input/Input.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Camera.hpp>
#include <renderer/CameraController.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Material.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Renderer2D.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/Shader.hpp>
#include <renderer/Texture.hpp>
#include <renderer/VertexArray.hpp>

#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace vshade::examples {

inline core::ApplicationConfig config(const char* title) {
    return {.window = {
        .title = title,
        .width = 960,
        .height = 540,
        .fullscreen = false,
        .vsync = true,
    }};
}

inline std::filesystem::path asset(const char* name) {
    return std::filesystem::path(VSHADE_EXAMPLE_SOURCE_DIR) / "assets" / name;
}

inline void closeOnEscape(core::Application& application) {
    if (input::Input::isKeyPressed(input::KeyCode::Escape)) application.close();
}

inline void clear(const math::Vec4& color = {0.025F, 0.035F, 0.06F, 1.0F}) {
    renderer::Renderer::setClearColor(color);
    renderer::Renderer::clear(
        renderer::ClearFlags::Color | renderer::ClearFlags::Depth
    );
}

inline renderer::Mesh triangleMesh() {
    constexpr std::array<renderer::MeshVertex, 3> vertices{{
        {{-0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{ 0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{ 0.0F,  0.7F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};
    auto verticesBuffer = std::make_shared<renderer::VertexBuffer>(
        vertices.data(), sizeof(vertices)
    );
    verticesBuffer->setLayout({
        {"position", renderer::ShaderDataType::Float3},
        {"normal", renderer::ShaderDataType::Float3},
        {"textureCoordinate", renderer::ShaderDataType::Float2},
        {"tangent", renderer::ShaderDataType::Float4},
    });
    auto indicesBuffer = std::make_shared<renderer::IndexBuffer>(
        indices.data(), indices.size()
    );
    auto array = std::make_shared<renderer::VertexArray>();
    array->addVertexBuffer(std::move(verticesBuffer));
    array->setIndexBuffer(std::move(indicesBuffer));
    return renderer::Mesh(std::move(array));
}

inline renderer::Mesh cubeMesh() {
    constexpr std::array<renderer::MeshVertex, 8> vertices{{
        {{-0.5F,-0.5F,-0.5F},{-0.577F,-0.577F,-0.577F},{0.0F,0.0F}},
        {{ 0.5F,-0.5F,-0.5F},{ 0.577F,-0.577F,-0.577F},{1.0F,0.0F}},
        {{ 0.5F, 0.5F,-0.5F},{ 0.577F, 0.577F,-0.577F},{1.0F,1.0F}},
        {{-0.5F, 0.5F,-0.5F},{-0.577F, 0.577F,-0.577F},{0.0F,1.0F}},
        {{-0.5F,-0.5F, 0.5F},{-0.577F,-0.577F, 0.577F},{0.0F,0.0F}},
        {{ 0.5F,-0.5F, 0.5F},{ 0.577F,-0.577F, 0.577F},{1.0F,0.0F}},
        {{ 0.5F, 0.5F, 0.5F},{ 0.577F, 0.577F, 0.577F},{1.0F,1.0F}},
        {{-0.5F, 0.5F, 0.5F},{-0.577F, 0.577F, 0.577F},{0.0F,1.0F}},
    }};
    constexpr std::array<std::uint32_t, 36> indices{
        0,2,1, 0,3,2, 4,5,6, 4,6,7,
        0,7,3, 0,4,7, 1,2,6, 1,6,5,
        3,7,6, 3,6,2, 0,1,5, 0,5,4,
    };
    auto verticesBuffer = std::make_shared<renderer::VertexBuffer>(
        vertices.data(), sizeof(vertices)
    );
    verticesBuffer->setLayout({
        {"position", renderer::ShaderDataType::Float3},
        {"normal", renderer::ShaderDataType::Float3},
        {"textureCoordinate", renderer::ShaderDataType::Float2},
        {"tangent", renderer::ShaderDataType::Float4},
    });
    auto indicesBuffer = std::make_shared<renderer::IndexBuffer>(
        indices.data(), indices.size()
    );
    auto array = std::make_shared<renderer::VertexArray>();
    array->addVertexBuffer(std::move(verticesBuffer));
    array->setIndexBuffer(std::move(indicesBuffer));
    return renderer::Mesh(std::move(array));
}

inline renderer::Camera perspectiveCamera() {
    renderer::Camera camera;
    camera.setPerspective(0.785398163F, 16.0F / 9.0F, 0.05F, 100.0F);
    camera.lookAt({3.0F, 2.0F, 4.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    return camera;
}

} // namespace vshade::examples
