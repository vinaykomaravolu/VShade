#include "renderer/DebugDraw.hpp"

#include "renderer/Buffer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Shader.hpp"
#include "renderer/VertexArray.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace vshade::renderer {
namespace {

struct DebugLine {
    math::Vec3 start;
    math::Vec3 end;
    math::Vec4 color;
};

struct DebugVertex {
    math::Vec3 position;
    math::Vec4 color;
};

struct DebugDrawResources {
    Shader shader;
    std::shared_ptr<VertexBuffer> vertexBuffer;
    VertexArray vertexArray;
    std::size_t vertexCapacity = 0;

    explicit DebugDrawResources(const std::size_t capacity)
        : shader(
              "vshade-debug-draw",
              R"glsl(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

uniform mat4 viewProjection;

out vec4 vertexColor;

void main() {
    vertexColor = color;
    gl_Position = viewProjection * vec4(position, 1.0);
}
)glsl",
              R"glsl(#version 330 core
in vec4 vertexColor;

out vec4 fragmentColor;

void main() {
    fragmentColor = vertexColor;
}
)glsl"
          ),
          vertexBuffer(std::make_shared<VertexBuffer>(capacity * sizeof(DebugVertex))),
          vertexCapacity(capacity) {
        vertexBuffer->setLayout({
            {"position", ShaderDataType::Float3},
            {"color", ShaderDataType::Float4},
        });
        vertexArray.addVertexBuffer(vertexBuffer);
    }
};

struct DebugDrawState {
    std::vector<DebugLine> lines;
    std::unique_ptr<DebugDrawResources> resources;
};

[[nodiscard]] DebugDrawState& state() {
    static DebugDrawState instance;
    return instance;
}

[[nodiscard]] bool finite(const math::Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool finite(const math::Vec4& value) noexcept {
    return finite(math::Vec3{value}) && std::isfinite(value.w);
}

void validateLine(
    const math::Vec3& start,
    const math::Vec3& end,
    const math::Vec4& color
) {
    if (!finite(start) || !finite(end) || !finite(color)) {
        throw std::invalid_argument("Debug line values must be finite");
    }
}

[[nodiscard]] DebugDrawResources& resources(const std::size_t vertexCount) {
    DebugDrawState& debugState = state();
    if (!debugState.resources || debugState.resources->vertexCapacity < vertexCount) {
        debugState.resources = std::make_unique<DebugDrawResources>(vertexCount);
    }
    return *debugState.resources;
}

} // namespace

void DebugDraw::line(
    const math::Vec3& start,
    const math::Vec3& end,
    const math::Vec4& color
) {
    validateLine(start, end, color);
    state().lines.push_back({start, end, color});
}

void DebugDraw::box(const DebugBounds& bounds, const math::Vec4& color) {
    if (!finite(bounds.minimum) || !finite(bounds.maximum) || !finite(color)) {
        throw std::invalid_argument("Debug box values must be finite");
    }
    if (bounds.minimum.x > bounds.maximum.x ||
        bounds.minimum.y > bounds.maximum.y ||
        bounds.minimum.z > bounds.maximum.z) {
        throw std::invalid_argument("Debug box minimum must not exceed maximum");
    }

    const std::array<math::Vec3, 8> corners{{
        {bounds.minimum.x, bounds.minimum.y, bounds.minimum.z},
        {bounds.maximum.x, bounds.minimum.y, bounds.minimum.z},
        {bounds.maximum.x, bounds.maximum.y, bounds.minimum.z},
        {bounds.minimum.x, bounds.maximum.y, bounds.minimum.z},
        {bounds.minimum.x, bounds.minimum.y, bounds.maximum.z},
        {bounds.maximum.x, bounds.minimum.y, bounds.maximum.z},
        {bounds.maximum.x, bounds.maximum.y, bounds.maximum.z},
        {bounds.minimum.x, bounds.maximum.y, bounds.maximum.z},
    }};
    constexpr std::array<std::array<std::size_t, 2>, 12> edges{{
        {{0, 1}}, {{1, 2}}, {{2, 3}}, {{3, 0}},
        {{4, 5}}, {{5, 6}}, {{6, 7}}, {{7, 4}},
        {{0, 4}}, {{1, 5}}, {{2, 6}}, {{3, 7}},
    }};

    for (const auto& edge : edges) {
        line(corners[edge[0]], corners[edge[1]], color);
    }
}

void DebugDraw::sphere(
    const math::Vec3& center,
    const float radius,
    const math::Vec4& color,
    const std::uint32_t segments
) {
    if (!finite(center) || !std::isfinite(radius) || !finite(color)) {
        throw std::invalid_argument("Debug sphere values must be finite");
    }
    if (radius <= 0.0F) {
        throw std::invalid_argument("Debug sphere radius must be positive");
    }
    if (segments < 3) {
        throw std::invalid_argument("Debug sphere requires at least three segments");
    }

    const float step = 2.0F * std::numbers::pi_v<float> / static_cast<float>(segments);
    for (std::uint32_t segment = 0; segment < segments; ++segment) {
        const float firstAngle = static_cast<float>(segment) * step;
        const float secondAngle = static_cast<float>(segment + 1) * step;
        const float firstCosine = std::cos(firstAngle) * radius;
        const float firstSine = std::sin(firstAngle) * radius;
        const float secondCosine = std::cos(secondAngle) * radius;
        const float secondSine = std::sin(secondAngle) * radius;

        line(
            center + math::Vec3{firstCosine, firstSine, 0.0F},
            center + math::Vec3{secondCosine, secondSine, 0.0F},
            color
        );
        line(
            center + math::Vec3{firstCosine, 0.0F, firstSine},
            center + math::Vec3{secondCosine, 0.0F, secondSine},
            color
        );
        line(
            center + math::Vec3{0.0F, firstCosine, firstSine},
            center + math::Vec3{0.0F, secondCosine, secondSine},
            color
        );
    }
}

void DebugDraw::flush(const Camera& camera) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("DebugDraw requires an initialized Renderer");
    }

    DebugDrawState& debugState = state();
    if (debugState.lines.empty()) {
        return;
    }

    std::vector<DebugVertex> vertices;
    vertices.reserve(debugState.lines.size() * 2);
    for (const DebugLine& debugLine : debugState.lines) {
        vertices.push_back({debugLine.start, debugLine.color});
        vertices.push_back({debugLine.end, debugLine.color});
    }

    DebugDrawResources& debugResources = resources(vertices.size());
    debugResources.vertexBuffer->setData(vertices.data(), vertices.size() * sizeof(DebugVertex));

    auto pipelineGuard = Renderer::pushPipelineState();
    Renderer::setBlending(true);
    Renderer::setBlendFunction(BlendFactor::SourceAlpha, BlendFactor::OneMinusSourceAlpha);
    Renderer::setFaceCulling(false);
    Renderer::setDepthTesting(true);
    Renderer::setDepthFunction(DepthFunction::LessOrEqual);
    Renderer::setDepthWrite(false);

    debugResources.shader.bind();
    debugResources.shader.setMat4("viewProjection", camera.viewProjection());
    Renderer::drawArrays(
        debugResources.vertexArray,
        PrimitiveTopology::Lines,
        vertices.size()
    );
    debugState.lines.clear();
}

void DebugDraw::clear() noexcept {
    state().lines.clear();
}

std::size_t DebugDraw::lineCount() noexcept {
    return state().lines.size();
}

void DebugDraw::shutdown() noexcept {
    DebugDrawState& debugState = state();
    debugState.lines.clear();
    debugState.resources.reset();
}

} // namespace vshade::renderer
