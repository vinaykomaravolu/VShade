#pragma once

#include "renderer/rendertypes.hpp"

#include <cstddef>

#include <glad/gl.h>

namespace vshade::renderer::opengl {

struct TextureFormatInfo {
    GLint internalFormat;
    GLenum dataFormat;
    std::size_t bytesPerPixel;
};

[[nodiscard]] GLenum bufferUsage(BufferUsage usage);
[[nodiscard]] GLenum primitiveTopology(PrimitiveTopology topology);
[[nodiscard]] GLenum shaderDataType(ShaderDataType type);
[[nodiscard]] bool isIntegerType(ShaderDataType type) noexcept;
[[nodiscard]] TextureFormatInfo textureFormat(TextureFormat format);
[[nodiscard]] GLenum textureFilter(TextureFilter filter);
[[nodiscard]] GLenum textureWrap(TextureWrap wrap);

} // namespace vshade::renderer::opengl
