#include "renderer/renderer.hpp"

#include "core/log.hpp"
#include "renderer/mesh.hpp"
#include "renderer/vertexarray.hpp"

#include "opengl/openglutils.hpp"

#include <limits>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace vshade::renderer {
namespace {

bool initialized = false;
RenderStats renderStats{};

void requireInitialized() {
    if (!initialized) {
        throw std::logic_error("Renderer is not initialized");
    }
}

GLsizei checkedDimension(const std::uint32_t value) {
    if (value > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Viewport dimension is too large for OpenGL");
    }
    return static_cast<GLsizei>(value);
}

GLsizei checkedIndexCount(const std::size_t count) {
    if (count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Index count is too large for OpenGL");
    }
    return static_cast<GLsizei>(count);
}

} // namespace

void Renderer::initialize() {
    if (initialized) {
        return;
    }
    if (glfwGetCurrentContext() == nullptr) {
        throw std::logic_error("An active OpenGL context is required before renderer initialization");
    }

    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    initialized = true;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const auto* device = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    ENGINE_INFO(
        "Initialized OpenGL {}.{} renderer: {} ({})",
        GLAD_VERSION_MAJOR(version),
        GLAD_VERSION_MINOR(version),
        device ? device : "unknown device",
        vendor ? vendor : "unknown vendor"
    );
}

void Renderer::shutdown() noexcept {
    if (!initialized) {
        return;
    }
    initialized = false;
    renderStats = {};
    ENGINE_INFO("Renderer shutdown complete");
}

bool Renderer::isInitialized() noexcept {
    return initialized;
}

void Renderer::beginFrame() noexcept {
    renderStats = {};
}

void Renderer::setViewport(const std::uint32_t width, const std::uint32_t height) {
    requireInitialized();
    glViewport(0, 0, checkedDimension(width), checkedDimension(height));
}

void Renderer::setClearColor(const math::Vec4& color) {
    requireInitialized();
    glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::clear() {
    requireInitialized();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setDepthTesting(const bool enabled) {
    requireInitialized();
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void Renderer::setDithering(const bool enabled) {
    requireInitialized();
    if (enabled) {
        glEnable(GL_DITHER);
    } else {
        glDisable(GL_DITHER);
    }
}

void Renderer::drawIndexed(
    const VertexArray& vertexArray,
    const PrimitiveTopology topology,
    const std::size_t indexCount
) {
    requireInitialized();
    const auto& indexBuffer = vertexArray.indexBuffer();
    if (!indexBuffer) {
        throw std::invalid_argument("Indexed drawing requires an index buffer");
    }

    const std::size_t drawCount = indexCount == 0 ? indexBuffer->count() : indexCount;
    if (drawCount > indexBuffer->count()) {
        throw std::out_of_range("Draw index count exceeds the index buffer");
    }

    vertexArray.bind();
    glDrawElements(
        opengl::primitiveTopology(topology),
        checkedIndexCount(drawCount),
        GL_UNSIGNED_INT,
        nullptr
    );

    ++renderStats.drawCalls;
    renderStats.indexCount += drawCount;
}

void Renderer::draw(const Mesh& mesh) {
    drawIndexed(*mesh.vertexArray(), mesh.topology());
}

const RenderStats& Renderer::stats() noexcept {
    return renderStats;
}

} // namespace vshade::renderer
