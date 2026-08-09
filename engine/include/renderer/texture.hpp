#pragma once

#include "renderer/rendertypes.hpp"

#include <cstddef>
#include <cstdint>

namespace vshade::renderer {

/** @brief Owns a two-dimensional OpenGL texture. */
class Texture2D final {
public:
    /** @brief Creates a texture and optionally uploads tightly packed pixel data. */
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

    /** @brief Replaces the complete tightly packed pixel image. */
    void setData(const void* data, std::size_t size);

    /** @brief Binds this texture to @p slot. */
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
