#if defined(_MSC_VER)
    #define _CRT_SECURE_NO_WARNINGS
    #pragma warning(push, 0)
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#include "renderer/Texture.hpp"

#include "renderer/Renderer.hpp"

#include "opengl/OpenGLUtils.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace vshade::renderer {
namespace {

void requireRenderer() {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Texture operation requires an initialized renderer");
    }
}

std::size_t textureByteSize(
    const std::uint32_t width,
    const std::uint32_t height,
    const std::size_t bytesPerPixel
) {
    const auto widthValue = static_cast<std::size_t>(width);
    const auto heightValue = static_cast<std::size_t>(height);
    if (heightValue != 0 && widthValue > std::numeric_limits<std::size_t>::max() / heightValue) {
        throw std::overflow_error("Texture dimensions are too large");
    }

    const std::size_t pixelCount = widthValue * heightValue;
    if (bytesPerPixel != 0 && pixelCount > std::numeric_limits<std::size_t>::max() / bytesPerPixel) {
        throw std::overflow_error("Texture byte size is too large");
    }
    return pixelCount * bytesPerPixel;
}

void deleteTexture(std::uint32_t& rendererId) noexcept {
    if (rendererId != 0 && Renderer::isInitialized()) {
        glDeleteTextures(1, &rendererId);
    }
    rendererId = 0;
}

} // namespace

Texture2D::Texture2D(
    const std::uint32_t width,
    const std::uint32_t height,
    const TextureFormat format,
    const void* data,
    const TextureFilter filter,
    const TextureWrap wrap
) : m_width(width),
    m_height(height),
    m_format(format),
    m_filter(filter),
    m_wrap(wrap) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before creating a texture");
    }
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Texture dimensions must be greater than zero");
    }
    if (width > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max()) ||
        height > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Texture dimensions are too large for OpenGL");
    }

    const auto formatInfo = opengl::textureFormat(format);
    const auto filterValue = static_cast<GLint>(opengl::textureFilter(filter));
    const auto wrapValue = static_cast<GLint>(opengl::textureWrap(wrap));

    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

    glGenTextures(1, &m_rendererId);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterValue);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterValue);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapValue);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapValue);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        formatInfo.internalFormat,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        formatInfo.dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
}

Texture2D::~Texture2D() {
    deleteTexture(m_rendererId);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0)),
      m_format(other.m_format),
      m_filter(other.m_filter),
      m_wrap(other.m_wrap) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        deleteTexture(m_rendererId);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_format = other.m_format;
        m_filter = other.m_filter;
        m_wrap = other.m_wrap;
    }
    return *this;
}

Texture2D Texture2D::fromFile(
    const std::filesystem::path& path,
    const TextureFilter filter,
    const TextureWrap wrap,
    const bool flipVertically
) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before loading a texture");
    }

    const std::string pathString = path.string();
    int width = 0;
    int height = 0;
    using PixelPointer = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
    PixelPointer pixels(
        stbi_load(pathString.c_str(), &width, &height, nullptr, STBI_rgb_alpha),
        &stbi_image_free
    );

    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load texture '" + pathString + "': " +
            (reason != nullptr ? reason : "unknown image error")
        );
    }
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("Texture image has invalid dimensions: " + pathString);
    }

    constexpr std::size_t channelCount = 4;
    const std::size_t rowSize = static_cast<std::size_t>(width) * channelCount;
    if (flipVertically) {
        for (int row = 0; row < height / 2; ++row) {
            auto* top = pixels.get() + static_cast<std::size_t>(row) * rowSize;
            auto* bottom = pixels.get() + static_cast<std::size_t>(height - 1 - row) * rowSize;
            std::swap_ranges(top, top + rowSize, bottom);
        }
    }

    return Texture2D(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        TextureFormat::RGBA8,
        pixels.get(),
        filter,
        wrap
    );
}

void Texture2D::setData(const void* data, const std::size_t size) {
    requireRenderer();
    if (data == nullptr) {
        throw std::invalid_argument("Texture data must not be null");
    }

    const auto formatInfo = opengl::textureFormat(m_format);
    const std::size_t expectedSize = textureByteSize(m_width, m_height, formatInfo.bytesPerPixel);
    if (size != expectedSize) {
        throw std::invalid_argument("Texture data size does not match its dimensions and format");
    }

    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        static_cast<GLsizei>(m_width),
        static_cast<GLsizei>(m_height),
        formatInfo.dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
}

void Texture2D::bind(const std::uint32_t slot) const {
    requireRenderer();
    if (slot >= Renderer::maximumTextureSlots()) {
        throw std::out_of_range("Texture slot exceeds the OpenGL limit");
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
}

std::uint32_t Texture2D::width() const noexcept {
    return m_width;
}

std::uint32_t Texture2D::height() const noexcept {
    return m_height;
}

TextureFormat Texture2D::format() const noexcept {
    return m_format;
}

TextureFilter Texture2D::filter() const noexcept {
    return m_filter;
}

TextureWrap Texture2D::wrap() const noexcept {
    return m_wrap;
}

void Texture2D::setFilter(const TextureFilter filter) {
    requireRenderer();
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
    const auto value = static_cast<GLint>(opengl::textureFilter(filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, value);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    m_filter = filter;
}

void Texture2D::setWrap(const TextureWrap wrap) {
    requireRenderer();
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, m_rendererId);
    const auto value = static_cast<GLint>(opengl::textureWrap(wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, value);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, value);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    m_wrap = wrap;
}

std::uint32_t Texture2D::rendererId() const noexcept {
    return m_rendererId;
}

} // namespace vshade::renderer
