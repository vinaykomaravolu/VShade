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

/** @brief Framebuffer attachments that can be cleared by Renderer::clear(). */
enum class ClearFlags : std::uint8_t {
    None = 0,
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2,
};

/** @brief Combines framebuffer clear flags. */
[[nodiscard]] constexpr ClearFlags operator|(const ClearFlags left, const ClearFlags right) noexcept {
    return static_cast<ClearFlags>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right)
    );
}

/** @brief Returns the clear flags shared by both operands. */
[[nodiscard]] constexpr ClearFlags operator&(const ClearFlags left, const ClearFlags right) noexcept {
    return static_cast<ClearFlags>(
        static_cast<std::uint8_t>(left) & static_cast<std::uint8_t>(right)
    );
}

/** @brief Adds clear flags to @p left. */
constexpr ClearFlags& operator|=(ClearFlags& left, const ClearFlags right) noexcept {
    left = left | right;
    return left;
}

/** @brief Factors used to combine source and destination fragment colors. */
enum class BlendFactor {
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    DestinationColor,
    OneMinusDestinationColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha,
};

/** @brief Faces that can be discarded during rasterization. */
enum class CullFace {
    Front,
    Back,
    FrontAndBack,
};

/** @brief Vertex winding considered to face toward the camera. */
enum class FrontFace {
    Clockwise,
    CounterClockwise,
};

/** @brief Rasterization mode used for polygon faces. */
enum class PolygonMode {
    Fill,
    Line,
    Point,
};

/** @brief Comparison used when testing fragment depth values. */
enum class DepthFunction {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
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
