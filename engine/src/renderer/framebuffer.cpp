#include "renderer/framebuffer.hpp"

#include "renderer/renderer.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

#include <glad/gl.h>

namespace vshade::renderer {
namespace {

GLsizei checkedDimension(const std::uint32_t value) {
    if (value == 0) {
        throw std::invalid_argument("Framebuffer dimensions must be greater than zero");
    }
    if (value > static_cast<std::uint32_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::overflow_error("Framebuffer dimension is too large for OpenGL");
    }
    return static_cast<GLsizei>(value);
}

std::size_t pixelByteCount(const std::uint32_t width, const std::uint32_t height) {
    constexpr std::size_t channelCount = 4;
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Framebuffer dimensions must be greater than zero");
    }
    const auto widthValue = static_cast<std::size_t>(width);
    const auto heightValue = static_cast<std::size_t>(height);
    if (heightValue > std::numeric_limits<std::size_t>::max() / widthValue) {
        throw std::overflow_error("Framebuffer pixel count is too large");
    }

    const std::size_t pixelCount = widthValue * heightValue;
    if (pixelCount > std::numeric_limits<std::size_t>::max() / channelCount) {
        throw std::overflow_error("Framebuffer readback is too large");
    }
    return pixelCount * channelCount;
}

} // namespace

Framebuffer::Framebuffer(const std::uint32_t width, const std::uint32_t height)
    : m_width(width),
      m_height(height) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer must be initialized before creating a framebuffer");
    }
    create();
}

Framebuffer::~Framebuffer() {
    release();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_rendererId(std::exchange(other.m_rendererId, 0)),
      m_colorAttachmentId(std::exchange(other.m_colorAttachmentId, 0)),
      m_depthStencilAttachmentId(std::exchange(other.m_depthStencilAttachmentId, 0)),
      m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0)) {}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        release();
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_colorAttachmentId = std::exchange(other.m_colorAttachmentId, 0);
        m_depthStencilAttachmentId = std::exchange(other.m_depthStencilAttachmentId, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
    }
    return *this;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);
    Renderer::setViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(const std::uint32_t width, const std::uint32_t height) {
    checkedDimension(width);
    checkedDimension(height);
    if (width == m_width && height == m_height) {
        return;
    }

    release();
    m_width = width;
    m_height = height;
    create();
}

std::vector<std::uint8_t> Framebuffer::readPixels() const {
    std::vector<std::uint8_t> pixels(pixelByteCount(m_width, m_height));

    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rendererId);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(
        0,
        0,
        checkedDimension(m_width),
        checkedDimension(m_height),
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));

    const std::size_t rowSize = static_cast<std::size_t>(m_width) * 4;
    for (std::uint32_t row = 0; row < m_height / 2; ++row) {
        const std::size_t topOffset = static_cast<std::size_t>(row) * rowSize;
        const std::size_t bottomOffset = static_cast<std::size_t>(m_height - 1 - row) * rowSize;
        std::swap_ranges(
            pixels.begin() + static_cast<std::ptrdiff_t>(topOffset),
            pixels.begin() + static_cast<std::ptrdiff_t>(topOffset + rowSize),
            pixels.begin() + static_cast<std::ptrdiff_t>(bottomOffset)
        );
    }

    return pixels;
}

std::uint32_t Framebuffer::width() const noexcept {
    return m_width;
}

std::uint32_t Framebuffer::height() const noexcept {
    return m_height;
}

std::uint32_t Framebuffer::rendererId() const noexcept {
    return m_rendererId;
}

std::uint32_t Framebuffer::colorAttachmentId() const noexcept {
    return m_colorAttachmentId;
}

void Framebuffer::create() {
    const GLsizei width = checkedDimension(m_width);
    const GLsizei height = checkedDimension(m_height);

    glGenFramebuffers(1, &m_rendererId);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);

    glGenTextures(1, &m_colorAttachmentId);
    glBindTexture(GL_TEXTURE_2D, m_colorAttachmentId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_colorAttachmentId,
        0
    );

    glGenRenderbuffers(1, &m_depthStencilAttachmentId);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencilAttachmentId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        m_depthStencilAttachmentId
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        release();
        throw std::runtime_error("Failed to create a complete OpenGL framebuffer");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::release() noexcept {
    if (Renderer::isInitialized()) {
        if (m_depthStencilAttachmentId != 0) {
            glDeleteRenderbuffers(1, &m_depthStencilAttachmentId);
        }
        if (m_colorAttachmentId != 0) {
            glDeleteTextures(1, &m_colorAttachmentId);
        }
        if (m_rendererId != 0) {
            glDeleteFramebuffers(1, &m_rendererId);
        }
    }

    m_depthStencilAttachmentId = 0;
    m_colorAttachmentId = 0;
    m_rendererId = 0;
}

} // namespace vshade::renderer
