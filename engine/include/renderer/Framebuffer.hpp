#pragma once

#include <cstdint>
#include <vector>

namespace vshade::renderer {

class Renderer;

/**
 * @brief Owns an offscreen RGBA8 framebuffer with a depth-stencil attachment.
 *
 * Pixel readback returns four channels per pixel in top-to-bottom row order,
 * which can be written directly by conventional image encoders.
 */
class Framebuffer final {
public:
    /**
     * @brief Creates a framebuffer with the specified dimensions.
     *
     * @param width Width of the framebuffer in pixels.
     * @param height Height of the framebuffer in pixels.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If either dimension is zero.
     * @throws std::overflow_error If a dimension cannot be represented by OpenGL.
     * @throws std::runtime_error If OpenGL cannot create complete attachments.
     */
    Framebuffer(std::uint32_t width, std::uint32_t height);

    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    /** @brief Binds this framebuffer and saves the current target and viewport. */
    void bind() const;

    /**
     * @brief Restores the target and viewport saved by the latest bind().
     * @throws std::logic_error If no matching bind exists or the renderer is inactive.
     */
    static void unbind();

    /**
     * @brief Recreates the attachments at a new non-zero size.
     * @param width New framebuffer width in pixels.
     * @param height New framebuffer height in pixels.
     * @throws std::invalid_argument If either dimension is zero.
     * @throws std::overflow_error If a dimension cannot be represented by OpenGL.
     * @throws std::runtime_error If OpenGL cannot create complete attachments.
     */
    void resize(std::uint32_t width, std::uint32_t height);

    /**
     * @brief Reads the RGBA8 color attachment into top-to-bottom CPU memory.
     * @return Four tightly packed bytes per pixel in top-to-bottom row order.
     * @throws std::overflow_error If the required CPU allocation is too large.
     */
    [[nodiscard]] std::vector<std::uint8_t> readPixels() const;

    /** @brief Clears the signed integer entity-ID attachment. */
    void clearEntityId(std::int32_t value) const;

    /** @brief Reads one signed entity ID using bottom-left framebuffer coordinates. */
    [[nodiscard]] std::int32_t readEntityId(
        std::uint32_t x,
        std::uint32_t y
    ) const;

    /** @brief Reads one depth sample in the range 0 through 1 using bottom-left coordinates. */
    [[nodiscard]] float readDepth(std::uint32_t x, std::uint32_t y) const;

    /**
     * @brief Returns the attachment width.
     * @return Width in pixels.
     */
    [[nodiscard]] std::uint32_t width() const noexcept;

    /**
     * @brief Returns the attachment height.
     * @return Height in pixels.
     */
    [[nodiscard]] std::uint32_t height() const noexcept;

    /**
     * @brief Returns the native framebuffer identifier.
     * @return OpenGL framebuffer identifier.
     */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

    /**
     * @brief Returns the color texture identifier.
     * @return OpenGL color-attachment texture identifier.
     */
    [[nodiscard]] std::uint32_t colorAttachmentId() const noexcept;

private:
    friend class Renderer;

    static void resetBindingStack() noexcept;
    void create();
    void release() noexcept;

    std::uint32_t m_rendererId = 0;
    std::uint32_t m_colorAttachmentId = 0;
    std::uint32_t m_entityIdAttachmentId = 0;
    std::uint32_t m_depthStencilAttachmentId = 0;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
};

} // namespace vshade::renderer
