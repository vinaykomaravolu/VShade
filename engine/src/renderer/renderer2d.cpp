#include "renderer/renderer2d.hpp"

#include "renderer/buffer.hpp"
#include "renderer/renderer.hpp"
#include "renderer/shader.hpp"
#include "renderer/vertexarray.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vshade::renderer {
namespace {

struct QuadVertex {
    math::Vec3 position;
    math::Vec2 textureCoordinate;
};

struct QuadCommand {
    math::Mat4 model{1.0F};
    math::Vec4 color{1.0F};
    math::Vec2 tiling{1.0F};
    const Texture2D* texture = nullptr;
    std::shared_ptr<Texture2D> retainedTexture;
    std::int32_t sortingLayer = 0;
};

struct Renderer2DState {
    math::Mat4 view{1.0F};
    math::Mat4 projection{1.0F};
    std::vector<QuadCommand> commands;
    Renderer2DStats currentStats{};
    Renderer2DStats completedStats{};
    bool sceneActive = false;
};

struct QuadResources {
    Shader shader;
    Texture2D whiteTexture;
    VertexArray vertexArray;

    QuadResources()
        : shader(
              "vshade-renderer2d",
              R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 tiling;

out vec2 vTexCoord;

void main() {
    vTexCoord = aTexCoord * tiling;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl",
              R"glsl(#version 330 core
in vec2 vTexCoord;

uniform sampler2D image;
uniform vec4 tint;

out vec4 fragmentColor;

void main() {
    fragmentColor = texture(image, vTexCoord) * tint;
}
)glsl"
          ),
          whiteTexture(
              1,
              1,
              TextureFormat::RGBA8,
              whitePixel.data(),
              TextureFilter::Nearest,
              TextureWrap::Repeat
          ) {
        auto vertexBuffer = std::make_shared<VertexBuffer>(vertices.data(), sizeof(vertices));
        vertexBuffer->setLayout({
            {"position", ShaderDataType::Float3},
            {"textureCoordinate", ShaderDataType::Float2},
        });

        auto indexBuffer = std::make_shared<IndexBuffer>(indices.data(), indices.size());
        vertexArray.addVertexBuffer(std::move(vertexBuffer));
        vertexArray.setIndexBuffer(std::move(indexBuffer));
        shader.setInt("image", 0);
    }

private:
    static constexpr std::array<std::uint8_t, 4> whitePixel{255, 255, 255, 255};
    static constexpr std::array<QuadVertex, 4> vertices{{
        {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F}},
        {{0.5F, -0.5F, 0.0F}, {1.0F, 0.0F}},
        {{0.5F, 0.5F, 0.0F}, {1.0F, 1.0F}},
        {{-0.5F, 0.5F, 0.0F}, {0.0F, 1.0F}},
    }};
    static constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 3, 0};
};

[[nodiscard]] Renderer2DState& state() {
    static Renderer2DState instance;
    return instance;
}

void requireActiveScene() {
    if (!state().sceneActive) {
        throw std::logic_error("Renderer2D requires an active scene");
    }
}

void queueQuad(
    const math::Transform& transform,
    const math::Vec4& color,
    const math::Vec2& tiling,
    const Texture2D* texture,
    std::shared_ptr<Texture2D> retainedTexture,
    const std::int32_t sortingLayer
) {
    requireActiveScene();
    Renderer2DState& rendererState = state();
    rendererState.commands.push_back({
        .model = transform.matrix(),
        .color = color,
        .tiling = tiling,
        .texture = texture,
        .retainedTexture = std::move(retainedTexture),
        .sortingLayer = sortingLayer,
    });
    ++rendererState.currentStats.quadCount;
}

void discardCurrentScene() noexcept {
    Renderer2DState& rendererState = state();
    rendererState.commands.clear();
    rendererState.currentStats = {};
    rendererState.sceneActive = false;
}

} // namespace

void Renderer2D::beginScene(const Camera& camera) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer2D requires an initialized Renderer");
    }
    Renderer2DState& rendererState = state();
    if (rendererState.sceneActive) {
        throw std::logic_error("Renderer2D scene is already active");
    }

    Renderer::setDepthTesting(false);
    Renderer::setDepthWrite(false);
    Renderer::setFaceCulling(false);
    Renderer::setBlending(true);
    Renderer::setBlendFunction(
        BlendFactor::SourceAlpha,
        BlendFactor::OneMinusSourceAlpha
    );

    rendererState.view = camera.view();
    rendererState.projection = camera.projection();
    rendererState.commands.clear();
    rendererState.currentStats = {};
    rendererState.sceneActive = true;
}

void Renderer2D::drawQuad(
    const math::Transform& transform,
    const math::Vec4& color,
    const std::int32_t sortingLayer
) {
    queueQuad(transform, color, {1.0F, 1.0F}, nullptr, nullptr, sortingLayer);
}

void Renderer2D::drawQuad(
    const math::Transform& transform,
    const Texture2D& texture,
    const math::Vec4& tint,
    const math::Vec2& tiling,
    const std::int32_t sortingLayer
) {
    queueQuad(transform, tint, tiling, &texture, nullptr, sortingLayer);
}

void Renderer2D::drawSprite(
    const math::Transform& transform,
    const SpriteRendererComponent& sprite
) {
    queueQuad(
        transform,
        sprite.color,
        sprite.tiling,
        sprite.texture.get(),
        sprite.texture,
        sprite.sortingLayer
    );
}

void Renderer2D::endScene() {
    requireActiveScene();
    Renderer2DState& rendererState = state();

    try {
        std::stable_sort(
            rendererState.commands.begin(),
            rendererState.commands.end(),
            [](const QuadCommand& left, const QuadCommand& right) {
                return left.sortingLayer < right.sortingLayer;
            }
        );

        if (!rendererState.commands.empty()) {
            QuadResources resources;
            resources.shader.setMat4("view", rendererState.view);
            resources.shader.setMat4("projection", rendererState.projection);

            for (const QuadCommand& command : rendererState.commands) {
                resources.shader.setMat4("model", command.model);
                resources.shader.setVec4("tint", command.color);
                resources.shader.setVec2("tiling", command.tiling);

                const Texture2D& texture = command.texture != nullptr
                    ? *command.texture
                    : resources.whiteTexture;
                texture.bind(0);
                Renderer::drawIndexed(resources.vertexArray);
                ++rendererState.currentStats.drawCalls;
            }
        }

        rendererState.completedStats = rendererState.currentStats;
        discardCurrentScene();
    } catch (...) {
        discardCurrentScene();
        throw;
    }
}

const Renderer2DStats& Renderer2D::stats() noexcept {
    return state().completedStats;
}

} // namespace vshade::renderer
