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
