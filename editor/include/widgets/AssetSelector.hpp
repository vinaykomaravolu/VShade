#pragma once

#include <asset/AssetReference.hpp>

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

/** @brief Reusable Inspector control for choosing or importing typed assets. */
class AssetSelector final {
public:
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
