#include "renderer/buffer.hpp"

#include "renderer/renderer.hpp"

#include "opengl/openglutils.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace vshade::renderer {
namespace {

void requireRenderer() {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before creating buffers");
    }
}

GLsizeiptr checkedSize(const std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())) {
        throw std::overflow_error("Buffer size is too large for OpenGL");
    }
    return static_cast<GLsizeiptr>(size);
}

GLintptr checkedOffset(const std::size_t offset) {
    if (offset > static_cast<std::size_t>(std::numeric_limits<GLintptr>::max())) {
        throw std::overflow_error("Buffer offset is too large for OpenGL");
    }
    return static_cast<GLintptr>(offset);
}

void deleteBuffer(std::uint32_t& rendererId) noexcept {
    if (rendererId != 0 && Renderer::isInitialized()) {
        glDeleteBuffers(1, &rendererId);
    }
    rendererId = 0;
}

} // namespace

std::uint32_t BufferElement::size() const {
    return shaderDataTypeSize(type);
}

std::uint32_t BufferElement::componentCount() const {
    return shaderDataTypeComponentCount(type);
}

BufferLayout::BufferLayout(std::initializer_list<BufferElement> elements)
    : m_elements(elements) {
    calculateOffsetsAndStride();
}

const std::vector<BufferElement>& BufferLayout::elements() const noexcept {
    return m_elements;
}

std::uint32_t BufferLayout::stride() const noexcept {
    return m_stride;
}

bool BufferLayout::empty() const noexcept {
    return m_elements.empty();
}

void BufferLayout::calculateOffsetsAndStride() {
    m_stride = 0;
    for (auto& element : m_elements) {
        element.offset = m_stride;
        m_stride += element.size();
    }
}

VertexBuffer::VertexBuffer(const void* data, const std::size_t size, const BufferUsage usage)
    : m_size(size) {
    requireRenderer();
    if (data == nullptr && size != 0) {
        throw std::invalid_argument("Vertex buffer data must not be null");
    }

    glGenBuffers(1, &m_rendererId);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererId);
    glBufferData(GL_ARRAY_BUFFER, checkedSize(size), data, opengl::bufferUsage(usage));
}

VertexBuffer::VertexBuffer(const std::size_t size)
    : m_size(size) {
    requireRenderer();
    glGenBuffers(1, &m_rendererId);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererId);
    glBufferData(GL_ARRAY_BUFFER, checkedSize(size), nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    deleteBuffer(m_rendererId);
}

VertexBuffer::VertexBuffer(VertexBuffer&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_size(std::exchange(other.m_size, 0)),
      m_layout(std::move(other.m_layout)) {}

VertexBuffer& VertexBuffer::operator=(VertexBuffer&& other) noexcept {
    if (this != &other) {
        deleteBuffer(m_rendererId);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_size = std::exchange(other.m_size, 0);
        m_layout = std::move(other.m_layout);
    }
    return *this;
}

void VertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererId);
}

void VertexBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::setData(const void* data, const std::size_t size, const std::size_t offset) {
    if (data == nullptr && size != 0) {
        throw std::invalid_argument("Vertex buffer data must not be null");
    }
    if (offset > m_size || size > m_size - offset) {
        throw std::out_of_range("Vertex buffer update exceeds its capacity");
    }

    bind();
    glBufferSubData(GL_ARRAY_BUFFER, checkedOffset(offset), checkedSize(size), data);
}

void VertexBuffer::setLayout(BufferLayout layout) {
    m_layout = std::move(layout);
}

const BufferLayout& VertexBuffer::layout() const noexcept {
    return m_layout;
}

std::size_t VertexBuffer::size() const noexcept {
    return m_size;
}

std::uint32_t VertexBuffer::rendererId() const noexcept {
    return m_rendererId;
}

IndexBuffer::IndexBuffer(
    const std::uint32_t* indices,
    const std::size_t count,
    const BufferUsage usage
) : m_count(count) {
    requireRenderer();
    if (indices == nullptr && count != 0) {
        throw std::invalid_argument("Index buffer data must not be null");
    }
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t)) {
        throw std::overflow_error("Index buffer is too large");
    }

    // GL_ELEMENT_ARRAY_BUFFER is stored in the currently bound VAO. Preserve
    // that binding while uploading this buffer so constructing an unrelated
    // index buffer cannot silently replace another mesh's indices.
    GLint previousIndexBuffer = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousIndexBuffer);

    glGenBuffers(1, &m_rendererId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        checkedSize(count * sizeof(std::uint32_t)),
        indices,
        opengl::bufferUsage(usage)
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(previousIndexBuffer));
}

IndexBuffer::~IndexBuffer() {
    deleteBuffer(m_rendererId);
}

IndexBuffer::IndexBuffer(IndexBuffer&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_count(std::exchange(other.m_count, 0)) {}

IndexBuffer& IndexBuffer::operator=(IndexBuffer&& other) noexcept {
    if (this != &other) {
        deleteBuffer(m_rendererId);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_count = std::exchange(other.m_count, 0);
    }
    return *this;
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererId);
}

void IndexBuffer::unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

std::size_t IndexBuffer::count() const noexcept {
    return m_count;
}

std::uint32_t IndexBuffer::rendererId() const noexcept {
    return m_rendererId;
}

} // namespace vshade::renderer
