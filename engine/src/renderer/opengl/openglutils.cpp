#include "openglutils.hpp"

#include <stdexcept>

namespace vshade::renderer::opengl {

GLenum bufferUsage(const BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Static: return GL_STATIC_DRAW;
        case BufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
    }
    throw std::invalid_argument("Unknown buffer usage");
}

GLenum blendFactor(const BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero: return GL_ZERO;
        case BlendFactor::One: return GL_ONE;
        case BlendFactor::SourceColor: return GL_SRC_COLOR;
        case BlendFactor::OneMinusSourceColor: return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DestinationColor: return GL_DST_COLOR;
        case BlendFactor::OneMinusDestinationColor: return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SourceAlpha: return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DestinationAlpha: return GL_DST_ALPHA;
        case BlendFactor::OneMinusDestinationAlpha: return GL_ONE_MINUS_DST_ALPHA;
    }
    throw std::invalid_argument("Unknown blend factor");
}

GLbitfield clearFlags(const ClearFlags flags) {
    constexpr auto knownFlags = ClearFlags::Color | ClearFlags::Depth | ClearFlags::Stencil;
    const auto value = static_cast<std::uint8_t>(flags);
    const auto knownValue = static_cast<std::uint8_t>(knownFlags);
    if ((value & static_cast<std::uint8_t>(~knownValue)) != 0) {
        throw std::invalid_argument("Unknown framebuffer clear flag");
    }

    GLbitfield result = 0;
    if ((flags & ClearFlags::Color) == ClearFlags::Color) {
        result |= GL_COLOR_BUFFER_BIT;
    }
    if ((flags & ClearFlags::Depth) == ClearFlags::Depth) {
        result |= GL_DEPTH_BUFFER_BIT;
    }
    if ((flags & ClearFlags::Stencil) == ClearFlags::Stencil) {
        result |= GL_STENCIL_BUFFER_BIT;
    }
    return result;
}

GLenum cullFace(const CullFace face) {
    switch (face) {
        case CullFace::Front: return GL_FRONT;
        case CullFace::Back: return GL_BACK;
        case CullFace::FrontAndBack: return GL_FRONT_AND_BACK;
    }
    throw std::invalid_argument("Unknown cull face");
}

GLenum depthFunction(const DepthFunction function) {
    switch (function) {
        case DepthFunction::Never: return GL_NEVER;
        case DepthFunction::Less: return GL_LESS;
        case DepthFunction::Equal: return GL_EQUAL;
        case DepthFunction::LessOrEqual: return GL_LEQUAL;
        case DepthFunction::Greater: return GL_GREATER;
        case DepthFunction::NotEqual: return GL_NOTEQUAL;
        case DepthFunction::GreaterOrEqual: return GL_GEQUAL;
        case DepthFunction::Always: return GL_ALWAYS;
    }
    throw std::invalid_argument("Unknown depth function");
}

GLenum frontFace(const FrontFace winding) {
    switch (winding) {
        case FrontFace::Clockwise: return GL_CW;
        case FrontFace::CounterClockwise: return GL_CCW;
    }
    throw std::invalid_argument("Unknown front-face winding");
}

GLenum polygonMode(const PolygonMode mode) {
    switch (mode) {
        case PolygonMode::Fill: return GL_FILL;
        case PolygonMode::Line: return GL_LINE;
        case PolygonMode::Point: return GL_POINT;
    }
    throw std::invalid_argument("Unknown polygon mode");
}

GLenum primitiveTopology(const PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::Triangles: return GL_TRIANGLES;
        case PrimitiveTopology::Lines: return GL_LINES;
        case PrimitiveTopology::Points: return GL_POINTS;
    }
    throw std::invalid_argument("Unknown primitive topology");
}

GLenum shaderDataType(const ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4:
            return GL_FLOAT;
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
            return GL_INT;
    }
    throw std::invalid_argument("Unknown shader data type");
}

bool isIntegerType(const ShaderDataType type) noexcept {
    switch (type) {
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
            return true;
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4:
            return false;
    }
    return false;
}

TextureFormatInfo textureFormat(const TextureFormat format) {
    switch (format) {
        case TextureFormat::Red8: return {GL_R8, GL_RED, 1};
        case TextureFormat::RGB8: return {GL_RGB8, GL_RGB, 3};
        case TextureFormat::RGBA8: return {GL_RGBA8, GL_RGBA, 4};
    }
    throw std::invalid_argument("Unknown texture format");
}

GLenum textureFilter(const TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear: return GL_LINEAR;
    }
    throw std::invalid_argument("Unknown texture filter");
}

GLenum textureWrap(const TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat: return GL_REPEAT;
        case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
    }
    throw std::invalid_argument("Unknown texture wrap");
}

} // namespace vshade::renderer::opengl
