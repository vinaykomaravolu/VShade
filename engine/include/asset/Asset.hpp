#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace vshade::asset {

/** @brief Stable identifier derived from an asset's normalized source path. */
using AssetId = std::uint64_t;

/** @brief Sentinel used by an empty asset handle. */
inline constexpr AssetId invalidAssetId = 0;

/** @brief Portable classification used by catalogs, importers, and selectors. */
enum class AssetType {
    Unknown,
    Texture,
    Model,
    Audio,
    Scene,
    Prefab,
    Shader,
    Mesh,
};

/** @brief Persistent information associated with one known project asset. */
struct AssetMetadata {
    AssetId id = invalidAssetId;
    std::filesystem::path sourcePath;
    AssetType type = AssetType::Unknown;

    bool operator==(const AssetMetadata&) const = default;
};

[[nodiscard]] const char* toString(AssetType type) noexcept;
[[nodiscard]] std::optional<AssetType> assetTypeFromString(std::string_view name);
[[nodiscard]] AssetType assetTypeFromExtension(const std::filesystem::path& extension);

} // namespace vshade::asset
