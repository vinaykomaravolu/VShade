#pragma once

#include "renderer/buffer.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace vshade::renderer {

/** @brief Connects vertex buffers, layouts, and an index buffer for drawing. */
class VertexArray final {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    /** @brief Binds this vertex array. */
    void bind() const;

    /** @brief Unbinds the current vertex array. */
    static void unbind();

    /**
     * @brief Adds a vertex buffer and configures its vertex attributes.
     * @param vertexBuffer Vertex buffer with a non-empty layout.
     */
    void addVertexBuffer(std::shared_ptr<VertexBuffer> vertexBuffer);

    /**
     * @brief Replaces the index buffer used by this vertex array.
     * @param indexBuffer Index buffer retained for indexed drawing.
     */
    void setIndexBuffer(std::shared_ptr<IndexBuffer> indexBuffer);

    /** @brief Returns all vertex buffers retained by this vertex array. */
    [[nodiscard]] const std::vector<std::shared_ptr<VertexBuffer>>& vertexBuffers() const noexcept;

    /** @brief Returns the retained index buffer, or null when none is set. */
    [[nodiscard]] const std::shared_ptr<IndexBuffer>& indexBuffer() const noexcept;

    /** @brief Returns the native OpenGL object identifier. */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::uint32_t m_nextAttributeIndex = 0;
    std::vector<std::shared_ptr<VertexBuffer>> m_vertexBuffers;
    std::shared_ptr<IndexBuffer> m_indexBuffer;
};

} // namespace vshade::renderer
