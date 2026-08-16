#pragma once

#include <cstdint>
#include <filesystem>

namespace vshade::asset {

/** @brief Stable identifier derived from an asset's normalized source path. */
using AssetId = std::uint64_t;

/** @brief Sentinel used by an empty asset handle. */
inline constexpr AssetId invalidAssetId = 0;

/** @brief Persistent information associated with one loaded asset. */
struct AssetMetadata {
    AssetId id = invalidAssetId;
    std::filesystem::path sourcePath;

    bool operator==(const AssetMetadata&) const = default;
};

} // namespace vshade::asset
