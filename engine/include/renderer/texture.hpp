#pragma once

#include "renderer/rendertypes.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>

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
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If either dimension is zero.
     * @throws std::overflow_error If the texture dimensions are too large.
     * @warning When @p data is non-null, it must contain enough tightly packed pixels for the format.
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
     * @brief Loads an image file and uploads it as an RGBA8 texture.
     * @param path Image file to decode with stb_image.
     * @param filter Sampling filter used when the texture is scaled.
     * @param wrap Texture-coordinate behavior outside zero through one.
     * @param flipVertically True to convert top-left image rows to OpenGL's bottom-left convention.
     * @return Texture containing the decoded image pixels.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::runtime_error If the image cannot be decoded.
     */
    [[nodiscard]] static Texture2D fromFile(
        const std::filesystem::path& path,
        TextureFilter filter = TextureFilter::Linear,
        TextureWrap wrap = TextureWrap::Repeat,
        bool flipVertically = true
    );

    /**
     * @brief Replaces the complete tightly packed pixel image.
     * @param data Source pixels to upload.
     * @param size Size of @p data in bytes.
     * @throws std::invalid_argument If @p data is null or @p size does not match the texture.
     */
    void setData(const void* data, std::size_t size);

    /**
     * @brief Binds this texture to @p slot.
     * @param slot Zero-based texture unit index.
     * @throws std::out_of_range If @p slot exceeds the OpenGL texture-unit limit.
     */
    void bind(std::uint32_t slot = 0) const;

    /**
     * @brief Returns the texture width.
     * @return Width in pixels.
     */
    [[nodiscard]] std::uint32_t width() const noexcept;

    /**
     * @brief Returns the texture height.
     * @return Height in pixels.
     */
    [[nodiscard]] std::uint32_t height() const noexcept;

    /**
     * @brief Returns the texture storage format.
     * @return Channel and storage format.
     */
    [[nodiscard]] TextureFormat format() const noexcept;

    /**
     * @brief Returns the native texture identifier.
     * @return OpenGL texture object identifier.
     */
    [[nodiscard]] std::uint32_t rendererId() const noexcept;

private:
    std::uint32_t m_rendererId = 0;
    std::uint32_t m_width = 0;
    std::uint32_t m_height = 0;
    TextureFormat m_format = TextureFormat::RGBA8;
};

} // namespace vshade::renderer
