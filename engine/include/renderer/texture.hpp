#pragma once

#include "renderer/rendertypes.hpp"

#include <cstddef>
#include <cstdint>

namespace vshade::renderer {

/** @brief Owns a two-dimensional OpenGL texture. */
class Texture2D final {
public:
    /**
     * @brief Creates a texture and optionally uploads tightly packed pixel data.
     * @param width Texture width in pixels.
     * @param height Texture height in pixels.
     * @param format Channel layout and storage format.
     * @param data Optional tightly packed source pixels.
     * @param filter Sampling filter used when the texture is scaled.
     * @param wrap Texture-coordinate behavior outside zero through one.
     */
    Texture2D(
        std::uint32_t width,
        std::uint32_t height,
        TextureFormat format = TextureFormat::RGBA8,
        const void* data = nullptr,
        TextureFilter filter = TextureFilter::Linear,
        TextureWrap wrap = TextureWrap::Repeat
    );

    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    /**
     * @brief Replaces the complete tightly packed pixel image.
     * @param data Source pixels to upload.
     * @param size Size of @p data in bytes.
     */
    void setData(const void* data, std::size_t size);

    /**
     * @brief Binds this texture to @p slot.
     * @param slot Zero-based texture unit index.
     */
    void bind(std::uint32_t slot = 0) const;

    [[nodiscard]] std::uint32_t width() const noexcept;
    [[nodiscard]] std::uint32_t height() const noexcept;
    [[nodiscard]] TextureFormat format() const noexcept;
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};

} // namespace vshade::renderer
