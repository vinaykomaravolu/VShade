#include "renderer/Framebuffer.hpp"

#include "renderer/Renderer.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glad/gl.h>

namespace vshade::renderer {
namespace {

struct FramebufferBindingState {
    GLint drawFramebuffer = 0;
    GLint readFramebuffer = 0;
    GLint viewport[4]{0, 0, 0, 0};
};

std::vector<FramebufferBindingState> bindingStack;

void requireRenderer() {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Framebuffer operation requires an initialized renderer");
    }
}

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
      m_entityIdAttachmentId(std::exchange(other.m_entityIdAttachmentId, 0)),
      m_depthStencilAttachmentId(std::exchange(other.m_depthStencilAttachmentId, 0)),
      m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0)) {}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        release();
        m_rendererId = std::exchange(other.m_rendererId, 0);
        m_colorAttachmentId = std::exchange(other.m_colorAttachmentId, 0);
        m_entityIdAttachmentId = std::exchange(other.m_entityIdAttachmentId, 0);
        m_depthStencilAttachmentId = std::exchange(other.m_depthStencilAttachmentId, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
    }
    return *this;
}

void Framebuffer::bind() const {
    requireRenderer();
    FramebufferBindingState previous;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous.drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous.readFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previous.viewport);
    bindingStack.push_back(previous);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rendererId);
    Renderer::setViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() {
    requireRenderer();
    if (bindingStack.empty()) {
        throw std::logic_error("Framebuffer::unbind requires a matching bind");
    }

    const FramebufferBindingState previous = bindingStack.back();
    bindingStack.pop_back();
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        static_cast<GLuint>(previous.drawFramebuffer)
    );
    glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        static_cast<GLuint>(previous.readFramebuffer)
    );
    Renderer::setViewport(
        static_cast<std::uint32_t>(std::max(previous.viewport[0], 0)),
        static_cast<std::uint32_t>(std::max(previous.viewport[1], 0)),
        static_cast<std::uint32_t>(std::max(previous.viewport[2], 0)),
        static_cast<std::uint32_t>(std::max(previous.viewport[3], 0))
    );
}

void Framebuffer::resize(const std::uint32_t width, const std::uint32_t height) {
    requireRenderer();
    checkedDimension(width);
    checkedDimension(height);
    if (width == m_width && height == m_height) {
        return;
    }

    GLint drawFramebuffer = 0;
    GLint readFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    if (drawFramebuffer == static_cast<GLint>(m_rendererId) ||
        readFramebuffer == static_cast<GLint>(m_rendererId) ||
        std::any_of(
            bindingStack.begin(),
            bindingStack.end(),
            [this](const FramebufferBindingState& binding) {
                return binding.drawFramebuffer == static_cast<GLint>(m_rendererId) ||
                    binding.readFramebuffer == static_cast<GLint>(m_rendererId);
            }
        )) {
        throw std::logic_error("Cannot resize a framebuffer while it is bound");
    }

    Framebuffer replacement(width, height);
    *this = std::move(replacement);
}

std::vector<std::uint8_t> Framebuffer::readPixels() const {
    requireRenderer();
    std::vector<std::uint8_t> pixels(pixelByteCount(m_width, m_height));

    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    GLint previousPackAlignment = 0;
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
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
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);

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

void Framebuffer::clearEntityId(const std::int32_t value) const {
    requireRenderer();
    GLint previousDrawFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_rendererId);
    const GLint clearValue = value;
    glClearBufferiv(GL_COLOR, 1, &clearValue);
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        static_cast<GLuint>(previousDrawFramebuffer)
    );
}

std::int32_t Framebuffer::readEntityId(
    const std::uint32_t x,
    const std::uint32_t y
) const {
    requireRenderer();
    if (x >= m_width || y >= m_height) {
        throw std::out_of_range("Framebuffer entity-ID coordinates are out of range");
    }

    GLint previousReadFramebuffer = 0;
    GLint previousReadBuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rendererId);
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    GLint value = -1;
    glReadPixels(
        static_cast<GLint>(x),
        static_cast<GLint>(y),
        1,
        1,
        GL_RED_INTEGER,
        GL_INT,
        &value
    );

    glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        static_cast<GLuint>(previousReadFramebuffer)
    );
    glReadBuffer(static_cast<GLenum>(previousReadBuffer));
    return static_cast<std::int32_t>(value);
}

float Framebuffer::readDepth(const std::uint32_t x, const std::uint32_t y) const {
    requireRenderer();
    if (x >= m_width || y >= m_height) {
        throw std::out_of_range("Framebuffer depth coordinates are out of range");
    }

    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rendererId);

    GLfloat depth = 1.0F;
    glReadPixels(
        static_cast<GLint>(x),
        static_cast<GLint>(y),
        1,
        1,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        &depth
    );

    glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        static_cast<GLuint>(previousReadFramebuffer)
    );
    return static_cast<float>(depth);
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

void Framebuffer::resetBindingStack() noexcept {
    bindingStack.clear();
}

void Framebuffer::create() {
    const GLsizei width = checkedDimension(m_width);
    const GLsizei height = checkedDimension(m_height);

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousTexture = 0;
    GLint previousRenderbuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

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

    glGenTextures(1, &m_entityIdAttachmentId);
    glBindTexture(GL_TEXTURE_2D, m_entityIdAttachmentId);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32I,
        width,
        height,
        0,
        GL_RED_INTEGER,
        GL_INT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT1,
        GL_TEXTURE_2D,
        m_entityIdAttachmentId,
        0
    );
    constexpr GLenum drawBuffers[]{
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };
    glDrawBuffers(2, drawBuffers);

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
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFramebuffer));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
        throw std::runtime_error("Failed to create a complete OpenGL framebuffer");
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(previousRenderbuffer));
}

void Framebuffer::release() noexcept {
    if (Renderer::isInitialized()) {
        if (m_depthStencilAttachmentId != 0) {
            glDeleteRenderbuffers(1, &m_depthStencilAttachmentId);
        }
        if (m_colorAttachmentId != 0) {
            glDeleteTextures(1, &m_colorAttachmentId);
        }
        if (m_entityIdAttachmentId != 0) {
            glDeleteTextures(1, &m_entityIdAttachmentId);
        }
        if (m_rendererId != 0) {
            glDeleteFramebuffers(1, &m_rendererId);
        }
    }

    m_depthStencilAttachmentId = 0;
    m_entityIdAttachmentId = 0;
    m_colorAttachmentId = 0;
    m_rendererId = 0;
}

} // namespace vshade::renderer
