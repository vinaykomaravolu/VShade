#pragma once

#include <cstdint>
#include <vector>

namespace vshade::renderer {

/**
 * @brief Owns an offscreen RGBA8 framebuffer with a depth-stencil attachment.
 *
 * Pixel readback returns four channels per pixel in top-to-bottom row order,
 * which can be written directly by conventional image encoders.
 */
class Framebuffer final {
public:
    /** @brief Creates a complete offscreen framebuffer at the requested size. */
    Framebuffer(std::uint32_t width, std::uint32_t height);

    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /** @brief Binds this framebuffer for drawing and updates the viewport. */
    void bind() const;

    /** @brief Binds the default framebuffer for drawing and reading. */
    static void unbind();

    /** @brief Recreates the attachments at a new non-zero size. */
    void resize(std::uint32_t width, std::uint32_t height);

    /** @brief Reads the RGBA8 color attachment into top-to-bottom CPU memory. */
    [[nodiscard]] std::vector<std::uint8_t> readPixels() const;

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] std::uint32_t rendererId() const noexcept;
    [[nodiscard]] std::uint32_t colorAttachmentId() const noexcept;

private:
    void create();
    void release() noexcept;

    std::uint32_t m_rendererId = 0;
    std::uint32_t m_colorAttachmentId = 0;
    std::uint32_t m_depthStencilAttachmentId = 0;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

} // namespace vshade::renderer
