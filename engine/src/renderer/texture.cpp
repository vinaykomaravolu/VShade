#include "renderer/texture.hpp"

#include "renderer/renderer.hpp"

#include "opengl/openglutils.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace vshade::renderer {
namespace {

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
    m_format(format) {
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
}

Texture2D::~Texture2D() {
    deleteTexture(m_rendererId);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0)),
      m_format(other.m_format) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        deleteTexture(m_rendererId);
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_format = other.m_format;
    }
    return *this;
}

void Texture2D::setData(const void* data, const std::size_t size) {
    if (data == nullptr) {
        throw std::invalid_argument("Texture data must not be null");
    }

    const auto formatInfo = opengl::textureFormat(m_format);
    const std::size_t expectedSize = textureByteSize(m_width, m_height, formatInfo.bytesPerPixel);
    if (size != expectedSize) {
        throw std::invalid_argument("Texture data size does not match its dimensions and format");
    }

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
}

void Texture2D::bind(const std::uint32_t slot) const {
    GLint maximumSlots = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maximumSlots);
    if (slot >= static_cast<std::uint32_t>(maximumSlots)) {
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

std::uint32_t Texture2D::rendererId() const noexcept {
    return m_rendererId;
}

} // namespace vshade::renderer
