#pragma once

#include "renderer/rendertypes.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace vshade::renderer {

/** @brief Describes one attribute stored in a vertex buffer. */
struct BufferElement {
    /** @brief Name used to describe the attribute while debugging. */
    std::string name;
    /** @brief Scalar type and component count. */
    ShaderDataType type = ShaderDataType::Float;
    /** @brief Whether integer data is normalized before reaching a float shader input. */
    bool normalized = false;
    /** @brief Byte offset calculated by BufferLayout. */
    std::uint32_t offset = 0;

    /** @brief Returns this element's size in bytes. */
    [[nodiscard]] std::uint32_t size() const;

    /** @brief Returns this element's number of scalar components. */
    [[nodiscard]] std::uint32_t componentCount() const;
};

/** @brief Ordered vertex attributes and their combined stride. */
class BufferLayout final {
public:
    BufferLayout() = default;

    /** @brief Creates a tightly packed layout from ordered elements. */
    BufferLayout(std::initializer_list<BufferElement> elements);

    /** @brief Returns the ordered vertex elements. */
    [[nodiscard]] const std::vector<BufferElement>& elements() const noexcept;

    /** @brief Returns the distance in bytes between consecutive vertices. */
    [[nodiscard]] std::uint32_t stride() const noexcept;

    /** @brief Returns whether this layout contains no attributes. */
    [[nodiscard]] bool empty() const noexcept;

private:
    void calculateOffsetsAndStride();

    std::vector<BufferElement> m_elements;
    std::uint32_t m_stride = 0;
};

/** @brief Owns an OpenGL vertex buffer. */
class VertexBuffer final {
public:
    /** @brief Uploads @p size bytes from @p data. */
    VertexBuffer(const void* data, std::size_t size, BufferUsage usage = BufferUsage::Static);

    /** @brief Allocates an empty dynamic vertex buffer. */
    explicit VertexBuffer(std::size_t size);

    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept;
    VertexBuffer& operator=(VertexBuffer&& other) noexcept;

    /** @brief Binds this buffer as the current vertex buffer. */
    void bind() const;

    /** @brief Unbinds the current vertex buffer. */
    static void unbind();

    /** @brief Replaces a byte range in this buffer. */
    void setData(const void* data, std::size_t size, std::size_t offset = 0);

    /** @brief Sets the structure of one vertex. */
    void setLayout(BufferLayout layout);

    /** @brief Returns the structure of one vertex. */
    [[nodiscard]] const BufferLayout& layout() const noexcept;

    /** @brief Returns the buffer capacity in bytes. */
    [[nodiscard]] std::size_t size() const noexcept;

    /** @brief Returns the native OpenGL object identifier. */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::size_t m_size = 0;
    BufferLayout m_layout;
};

/** @brief Owns a 32-bit OpenGL index buffer. */
class IndexBuffer final {
public:
    /** @brief Uploads @p count unsigned 32-bit indices from @p indices. */
    IndexBuffer(
        const std::uint32_t* indices,
        std::size_t count,
        BufferUsage usage = BufferUsage::Static
    );

    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&& other) noexcept;
    IndexBuffer& operator=(IndexBuffer&& other) noexcept;

    /** @brief Binds this buffer as the current index buffer. */
    void bind() const;

    /** @brief Unbinds the current index buffer. */
    static void unbind();

    /** @brief Returns the number of stored indices. */
    [[nodiscard]] std::size_t count() const noexcept;

    /** @brief Returns the native OpenGL object identifier. */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::size_t m_count = 0;
};

} // namespace vshade::renderer
