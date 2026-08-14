#pragma once

#include "renderer/RenderTypes.hpp"

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

    /**
     * @brief Returns this element's size in bytes.
     * @return Storage size of one attribute value.
     */
    [[nodiscard]] std::uint32_t size() const;

    /**
     * @brief Returns this element's number of scalar components.
     * @return Scalar component count represented by type.
     */
    [[nodiscard]] std::uint32_t componentCount() const;
};

/** @brief Ordered vertex attributes and their combined stride. */
class BufferLayout final {
public:
    BufferLayout() = default;

    /**
     * @brief Creates a tightly packed layout from ordered elements.
     * @param elements Vertex attributes in memory order.
     */
    BufferLayout(std::initializer_list<BufferElement> elements);

    /**
     * @brief Returns the ordered vertex elements.
     * @return Read-only reference to the calculated layout elements.
     */
    [[nodiscard]] const std::vector<BufferElement>& elements() const noexcept;

    /**
     * @brief Returns the distance in bytes between consecutive vertices.
     * @return Total byte stride of one vertex.
     */
    [[nodiscard]] std::uint32_t stride() const noexcept;

    /**
     * @brief Returns whether this layout contains no attributes.
     * @return True when the layout has no elements; otherwise false.
     */
    [[nodiscard]] bool empty() const noexcept;

private:
    void calculateOffsetsAndStride();

    std::vector<BufferElement> m_elements;
    std::uint32_t m_stride = 0;
};

/** @brief Owns an OpenGL vertex buffer. */
class VertexBuffer final {
public:
    /**
     * @brief Uploads @p size bytes from @p data.
     * @param data Source bytes to upload.
     * @param size Number of bytes to allocate and upload.
     * @param usage Expected GPU update frequency.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p data is null while @p size is non-zero.
     * @throws std::overflow_error If @p size cannot be represented by OpenGL.
     */
    VertexBuffer(const void* data, std::size_t size, BufferUsage usage = BufferUsage::Static);

    /**
     * @brief Allocates an empty dynamic vertex buffer.
     * @param size Number of bytes to allocate.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::overflow_error If @p size cannot be represented by OpenGL.
     */
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

    /**
     * @brief Replaces a byte range in this buffer.
     * @param data Source bytes to upload.
     * @param size Number of bytes to replace.
     * @param offset Destination byte offset within the buffer.
     * @throws std::invalid_argument If @p data is null while @p size is non-zero.
     * @throws std::out_of_range If the requested byte range exceeds the buffer capacity.
     * @throws std::overflow_error If the byte range cannot be represented by OpenGL.
     */
    void setData(const void* data, std::size_t size, std::size_t offset = 0);

    /**
     * @brief Sets the structure of one vertex.
     * @param layout Ordered attributes and stride for one vertex.
     */
    void setLayout(BufferLayout layout);

    /**
     * @brief Returns the structure of one vertex.
     * @return Read-only reference to this buffer's layout.
     */
    [[nodiscard]] const BufferLayout& layout() const noexcept;

    /**
     * @brief Returns the buffer capacity in bytes.
     * @return Allocated byte capacity.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Returns the native OpenGL object identifier.
     * @return OpenGL buffer object identifier.
     */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::size_t m_size = 0;
    BufferLayout m_layout;
};

/** @brief Owns a 32-bit OpenGL index buffer. */
class IndexBuffer final {
public:
    /**
     * @brief Uploads @p count unsigned 32-bit indices from @p indices.
     * @param indices Source indices to upload.
     * @param count Number of indices in @p indices.
     * @param usage Expected GPU update frequency.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p indices is null while @p count is non-zero.
     * @throws std::overflow_error If the requested storage cannot be represented.
     */
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

    /**
     * @brief Returns the number of stored indices.
     * @return Number of 32-bit indices in the buffer.
     */
    [[nodiscard]] std::size_t count() const noexcept;

    /**
     * @brief Returns the native OpenGL object identifier.
     * @return OpenGL buffer object identifier.
     */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::size_t m_count = 0;
};

} // namespace vshade::renderer
