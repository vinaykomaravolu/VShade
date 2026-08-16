#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::renderer {
class Texture2D;
}

namespace vshade::asset {

/** @brief Loads image files into runtime two-dimensional textures. */
class TextureLoader final : public AssetLoader<renderer::Texture2D> {
public:
    TextureLoader() = default;
    ~TextureLoader() override = default;

    /** @brief Decodes an image and creates its renderer texture resource. */
    [[nodiscard]] std::shared_ptr<renderer::Texture2D> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
