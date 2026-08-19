#pragma once

#include <asset/AssetReference.hpp>

#include <filesystem>

namespace vshade::asset {
class AssetManager;
}

namespace vshade::audio {
class AudioClip;
}

namespace vshade::renderer {
class Model;
class Texture2D;
}

namespace editor {

inline constexpr const char* assetDragDropType = "VShadeAsset";

/** @brief Reusable Inspector control for choosing or importing typed assets. */
class AssetSelector final {
public:
    static void setSearchDirectory(std::filesystem::path directory);
    static void discover(vshade::asset::AssetManager& assets);
    static bool acceptDroppedAsset(
        vshade::asset::AssetReference<vshade::renderer::Texture2D>& reference,
        vshade::asset::AssetManager& assets
    );
    static bool acceptDroppedAsset(
        vshade::asset::AssetReference<vshade::renderer::Model>& reference,
        vshade::asset::AssetManager& assets
    );
    static bool acceptDroppedAsset(
        vshade::asset::AssetReference<vshade::audio::AudioClip>& reference,
        vshade::asset::AssetManager& assets
    );

    static bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::renderer::Texture2D>& reference,
        vshade::asset::AssetManager& assets
    );

    static bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::renderer::Model>& reference,
        vshade::asset::AssetManager& assets
    );

    static bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::audio::AudioClip>& reference,
        vshade::asset::AssetManager& assets
    );
};

} // namespace editor
