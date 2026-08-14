#include "renderer/Renderer.hpp"

#include "core/Log.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Framebuffer.hpp"
#include "renderer/Renderer2D.hpp"
#include "renderer/Renderer3D.hpp"
#include "renderer/Shader.hpp"
#include "renderer/VertexArray.hpp"

#include "opengl/OpenGLUtils.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace vshade::renderer {
namespace {

bool initialized = false;
RenderStats renderStats{};
PipelineState currentPipelineState{};
Viewport currentViewport{};
std::uint32_t cachedMaximumTextureSlots = 0;

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

GLint checkedViewportPosition(const std::uint32_t value) {
    if (value > static_cast<std::uint32_t>(std::numeric_limits<GLint>::max())) {
        throw std::overflow_error("Viewport position is too large for OpenGL");
    }
    return static_cast<GLint>(value);
}

GLsizei checkedDrawCount(const std::size_t count) {
    if (count > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Draw count is too large for OpenGL");
    }
    return static_cast<GLsizei>(count);
}

GLint checkedFirstVertex(const std::size_t firstVertex) {
    if (firstVertex > static_cast<std::size_t>(std::numeric_limits<GLint>::max())) {
        throw std::overflow_error("First vertex is too large for OpenGL");
    }
    return static_cast<GLint>(firstVertex);
}

void setCapability(const GLenum capability, const bool enabled) {
    if (enabled) {
        glEnable(capability);
    } else {
        glDisable(capability);
    }
}

} // namespace

PipelineStateGuard::PipelineStateGuard(const PipelineState& state) noexcept
    : m_state(state) {}

PipelineStateGuard::~PipelineStateGuard() noexcept {
    restore();
}

PipelineStateGuard::PipelineStateGuard(PipelineStateGuard&& other) noexcept
    : m_state(other.m_state),
      m_active(std::exchange(other.m_active, false)) {}

PipelineStateGuard& PipelineStateGuard::operator=(PipelineStateGuard&& other) noexcept {
    if (this != &other) {
        restore();
        m_state = other.m_state;
        m_active = std::exchange(other.m_active, false);
    }
    return *this;
}

void PipelineStateGuard::restore() noexcept {
    if (!m_active) {
        return;
    }

    m_active = false;
    if (!Renderer::isInitialized()) {
        return;
    }

    try {
        Renderer::applyPipelineState(m_state);
    } catch (...) {
        ENGINE_ERROR("Failed to restore a scoped renderer pipeline state");
    }
}

bool PipelineStateGuard::active() const noexcept {
    return m_active;
}

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
    GLint textureSlots = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &textureSlots);
    cachedMaximumTextureSlots = textureSlots > 0
        ? static_cast<std::uint32_t>(textureSlots)
        : 0;

    setBlending(true);
    setBlendFunction(BlendFactor::SourceAlpha, BlendFactor::OneMinusSourceAlpha);
    setFaceCulling(false);
    setCullFace(CullFace::Back);
    setFrontFace(FrontFace::CounterClockwise);
    setPolygonMode(PolygonMode::Fill);
    setDepthTesting(true);
    setDepthFunction(DepthFunction::Less);
    setDepthWrite(true);
    setDithering(true);

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
    Renderer2D::shutdown();
    Renderer3D::shutdown();
    Framebuffer::resetBindingStack();
    Shader::resetBindingCache();
    initialized = false;
    renderStats = {};
    currentPipelineState = {};
    currentViewport = {};
    cachedMaximumTextureSlots = 0;
    ENGINE_INFO("Renderer shutdown complete");
}

bool Renderer::isInitialized() noexcept {
    return initialized;
}

void Renderer::beginFrame() noexcept {
    renderStats = {};
}

void Renderer::endFrame() {
    requireInitialized();
#if !defined(NDEBUG)
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error(
            "OpenGL error detected at end of frame: " + std::to_string(error)
        );
    }
#endif
}

PipelineState Renderer::pipelineState() noexcept {
    return currentPipelineState;
}

PipelineStateGuard Renderer::pushPipelineState() noexcept {
    return PipelineStateGuard(currentPipelineState);
}

void Renderer::applyPipelineState(const PipelineState& state) {
    setBlending(state.blending);
    setBlendFunction(state.sourceBlend, state.destinationBlend);
    setFaceCulling(state.faceCulling);
    setCullFace(state.cullFace);
    setFrontFace(state.frontFace);
    setPolygonMode(state.polygonMode);
    setDepthTesting(state.depthTesting);
    setDepthFunction(state.depthFunction);
    setDepthWrite(state.depthWrite);
    setDithering(state.dithering);
}

void Renderer::setViewport(
    const std::uint32_t x,
    const std::uint32_t y,
    const std::uint32_t width,
    const std::uint32_t height
) {
    requireInitialized();
    glViewport(
        checkedViewportPosition(x),
        checkedViewportPosition(y),
        checkedDimension(width),
        checkedDimension(height)
    );
    currentViewport = {x, y, width, height};
}

Viewport Renderer::viewport() noexcept {
    return currentViewport;
}

void Renderer::setClearColor(const math::Vec4& color) {
    requireInitialized();
    glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::clear(const ClearFlags flags) {
    requireInitialized();
    const GLbitfield mask = opengl::clearFlags(flags);

    // glClear respects GL_DEPTH_WRITEMASK. Overlays such as Renderer2D disable
    // depth writes, so make a requested depth clear reliable without changing
    // the caller's persistent render state.
    GLboolean depthWriteEnabled = GL_TRUE;
    if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
        if (depthWriteEnabled == GL_FALSE) {
            glDepthMask(GL_TRUE);
        }
    }

    glClear(mask);

    if ((mask & GL_DEPTH_BUFFER_BIT) != 0 && depthWriteEnabled == GL_FALSE) {
        glDepthMask(GL_FALSE);
    }
}

void Renderer::setBlending(const bool enabled) {
    requireInitialized();
    setCapability(GL_BLEND, enabled);
    currentPipelineState.blending = enabled;
}

void Renderer::setBlendFunction(const BlendFactor source, const BlendFactor destination) {
    requireInitialized();
    glBlendFunc(opengl::blendFactor(source), opengl::blendFactor(destination));
    currentPipelineState.sourceBlend = source;
    currentPipelineState.destinationBlend = destination;
}

void Renderer::setFaceCulling(const bool enabled) {
    requireInitialized();
    setCapability(GL_CULL_FACE, enabled);
    currentPipelineState.faceCulling = enabled;
}

void Renderer::setCullFace(const CullFace face) {
    requireInitialized();
    glCullFace(opengl::cullFace(face));
    currentPipelineState.cullFace = face;
}

void Renderer::setFrontFace(const FrontFace winding) {
    requireInitialized();
    glFrontFace(opengl::frontFace(winding));
    currentPipelineState.frontFace = winding;
}

void Renderer::setPolygonMode(const PolygonMode mode) {
    requireInitialized();
    glPolygonMode(GL_FRONT_AND_BACK, opengl::polygonMode(mode));
    currentPipelineState.polygonMode = mode;
}

void Renderer::setDepthTesting(const bool enabled) {
    requireInitialized();
    setCapability(GL_DEPTH_TEST, enabled);
    currentPipelineState.depthTesting = enabled;
}

void Renderer::setDepthFunction(const DepthFunction function) {
    requireInitialized();
    glDepthFunc(opengl::depthFunction(function));
    currentPipelineState.depthFunction = function;
}

void Renderer::setDepthWrite(const bool enabled) {
    requireInitialized();
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    currentPipelineState.depthWrite = enabled;
}

void Renderer::setDithering(const bool enabled) {
    requireInitialized();
    setCapability(GL_DITHER, enabled);
    currentPipelineState.dithering = enabled;
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
        checkedDrawCount(drawCount),
        GL_UNSIGNED_INT,
        nullptr
    );

    ++renderStats.drawCalls;
    renderStats.indexCount += drawCount;
}

void Renderer::drawArrays(
    const VertexArray& vertexArray,
    const PrimitiveTopology topology,
    const std::size_t vertexCount,
    const std::size_t firstVertex
) {
    requireInitialized();
    if (vertexArray.vertexBuffers().empty()) {
        throw std::invalid_argument("Array drawing requires a vertex buffer");
    }
    if (vertexCount == 0) {
        throw std::invalid_argument("Array drawing requires at least one vertex");
    }
    if (firstVertex > std::numeric_limits<std::size_t>::max() - vertexCount) {
        throw std::overflow_error("Array draw range is too large");
    }

    const std::size_t endVertex = firstVertex + vertexCount;
    for (const auto& vertexBuffer : vertexArray.vertexBuffers()) {
        const std::size_t stride = vertexBuffer->layout().stride();
        const std::size_t availableVertices = vertexBuffer->size() / stride;
        if (endVertex > availableVertices) {
            throw std::out_of_range("Array draw range exceeds a vertex buffer");
        }
    }

    vertexArray.bind();
    glDrawArrays(
        opengl::primitiveTopology(topology),
        checkedFirstVertex(firstVertex),
        checkedDrawCount(vertexCount)
    );

    ++renderStats.drawCalls;
    renderStats.vertexCount += vertexCount;
}

void Renderer::draw(const Mesh& mesh) {
    drawIndexed(*mesh.vertexArray(), mesh.topology());
}

const RenderStats& Renderer::stats() noexcept {
    return renderStats;
}

std::uint32_t Renderer::maximumTextureSlots() {
    requireInitialized();
    return cachedMaximumTextureSlots;
}

} // namespace vshade::renderer
