#pragma once

#include <cstdint>

namespace vshade::renderer {

/** @brief Data types supported by vertex buffer layouts. */
enum class ShaderDataType {
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
};

/** @brief Expected update frequency for GPU buffers. */
enum class BufferUsage {
    Static,
    Dynamic,
};

/** @brief Primitive assembly mode used by a draw call. */
enum class PrimitiveTopology {
    Triangles,
    Lines,
    Points,
};

/** @brief Pixel formats supported by Texture2D. */
enum class TextureFormat {
    Red8,
    RGB8,
    RGBA8,
};

/** @brief Texture sampling filters. */
enum class TextureFilter {
    Nearest,
    Linear,
};

/** @brief Texture coordinate behavior outside the zero-to-one range. */
enum class TextureWrap {
    Repeat,
    ClampToEdge,
};

/** @brief Returns the size of one value of @p type in bytes. */
[[nodiscard]] std::uint32_t shaderDataTypeSize(ShaderDataType type);

/** @brief Returns the scalar component count of @p type. */
[[nodiscard]] std::uint32_t shaderDataTypeComponentCount(ShaderDataType type);

} // namespace vshade::renderer
