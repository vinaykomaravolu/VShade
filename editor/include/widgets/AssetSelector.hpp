#pragma once

#include <asset/AssetReference.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

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
    struct State;

    AssetSelector();
    ~AssetSelector();

    AssetSelector(const AssetSelector&) = delete;
    AssetSelector& operator=(const AssetSelector&) = delete;

    void setSearchDirectory(std::filesystem::path directory);
    void setCatalogChangedCallback(std::function<void()> callback);
    void setImportCallback(
        std::function<std::optional<std::filesystem::path>(
            const std::filesystem::path&
        )> callback
    );
    void discover(vshade::asset::AssetManager& assets);
    [[nodiscard]] static std::optional<std::filesystem::path> acceptDroppedPath();
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

    bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::renderer::Texture2D>& reference,
        vshade::asset::AssetManager& assets
    );

    bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::renderer::Model>& reference,
        vshade::asset::AssetManager& assets
    );

    bool draw(
        const char* label,
        vshade::asset::AssetReference<vshade::audio::AudioClip>& reference,
        vshade::asset::AssetManager& assets
    );

private:
    std::unique_ptr<State> m_state;
};

} // namespace editor
