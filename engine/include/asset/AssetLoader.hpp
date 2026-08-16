#pragma once

#include <filesystem>
#include <memory>

namespace vshade::asset {

/**
 * @brief Interface that constructs one runtime resource type from an asset file.
 *
 * AssetManager installs the engine's built-in implementations automatically
 * and accepts replacements or custom implementations through
 * registerAssetLoader. Keeping this interface typed hides the manager's
 * type-erased internal storage from custom loaders.
 */
template<typename Resource>
class AssetLoader {
public:
    virtual ~AssetLoader() = default;

    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;
    AssetLoader(AssetLoader&&) = delete;
    AssetLoader& operator=(AssetLoader&&) = delete;

    /**
     * @brief Reads @p path and constructs its runtime resource.
     * @return Shared resource owned by the caller, or null when loading fails.
     */
    [[nodiscard]] virtual std::shared_ptr<Resource> load(
        const std::filesystem::path& path
    ) const = 0;

protected:
    AssetLoader() = default;
};

} // namespace vshade::asset
