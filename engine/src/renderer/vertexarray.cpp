#include "renderer/vertexarray.hpp"

#include "renderer/renderer.hpp"

#include "opengl/openglutils.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace vshade::renderer {
namespace {

void deleteVertexArray(std::uint32_t& rendererId) noexcept {
    if (rendererId != 0 && Renderer::isInitialized()) {
        glDeleteVertexArrays(1, &rendererId);
    }
    rendererId = 0;
}

} // namespace

VertexArray::VertexArray() {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before creating a vertex array");
    }
    glGenVertexArrays(1, &m_rendererId);
}

VertexArray::~VertexArray() {
    deleteVertexArray(m_rendererId);
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_nextAttributeIndex(std::exchange(other.m_nextAttributeIndex, 0)),
      m_vertexBuffers(std::move(other.m_vertexBuffers)),
      m_indexBuffer(std::move(other.m_indexBuffer)) {}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept {
    if (this != &other) {
        deleteVertexArray(m_rendererId);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_nextAttributeIndex = std::exchange(other.m_nextAttributeIndex, 0);
        m_vertexBuffers = std::move(other.m_vertexBuffers);
        m_indexBuffer = std::move(other.m_indexBuffer);
    }
    return *this;
}

void VertexArray::bind() const {
    glBindVertexArray(m_rendererId);
}

void VertexArray::unbind() {
    glBindVertexArray(0);
}

void VertexArray::addVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer) {
    if (!vertexBuffer) {
        throw std::invalid_argument("Vertex buffer must not be null");
    }
    if (vertexBuffer->layout().empty()) {
        throw std::invalid_argument("Vertex buffer must have a layout before it is added");
    }
    if (vertexBuffer->layout().stride() > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Vertex layout stride is too large for OpenGL");
    }

    bind();
    vertexBuffer->bind();

    for (const auto& element : vertexBuffer->layout().elements()) {
        glEnableVertexAttribArray(m_nextAttributeIndex);
        const auto componentCount = static_cast<GLint>(element.componentCount());
        const auto type = opengl::shaderDataType(element.type);
        const auto stride = static_cast<GLsizei>(vertexBuffer->layout().stride());
        const auto* offset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(element.offset));

        if (opengl::isIntegerType(element.type)) {
            glVertexAttribIPointer(m_nextAttributeIndex, componentCount, type, stride, offset);
        } else {
            glVertexAttribPointer(
                m_nextAttributeIndex,
                componentCount,
                type,
                element.normalized ? GL_TRUE : GL_FALSE,
                stride,
                offset
            );
        }
        ++m_nextAttributeIndex;
    }

    m_vertexBuffers.push_back(std::move(vertexBuffer));
}

void VertexArray::setIndexBuffer(std::shared_ptr<IndexBuffer> indexBuffer) {
    if (!indexBuffer) {
        throw std::invalid_argument("Index buffer must not be null");
    }
    bind();
    indexBuffer->bind();
    m_indexBuffer = std::move(indexBuffer);
}

const std::vector<std::shared_ptr<VertexBuffer>>& VertexArray::vertexBuffers() const noexcept {
    return m_vertexBuffers;
}

const std::shared_ptr<IndexBuffer>& VertexArray::indexBuffer() const noexcept {
    return m_indexBuffer;
}

std::uint32_t VertexArray::rendererId() const noexcept {
    return m_rendererId;
}

} // namespace vshade::renderer
