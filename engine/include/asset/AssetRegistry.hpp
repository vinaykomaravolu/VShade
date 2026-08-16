#pragma once

#include "asset/Asset.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace vshade::asset {

/**
 * @brief Catalog of stable asset identifiers and their source paths.
 *
 * The registry describes known project assets independently of whether their
 * runtime resources are currently loaded by AssetManager. Its implementation
 * will later enforce unique identifiers and normalized paths.
 */
class AssetRegistry final {
public:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;
    AssetRegistry(AssetRegistry&&) noexcept = default;
    AssetRegistry& operator=(AssetRegistry&&) noexcept = default;

    /**
     * @brief Adds metadata for a known asset.
     * @throws std::invalid_argument If its ID or path is invalid.
     * @throws std::logic_error If its ID or normalized path is already registered.
     */
    void registerAsset(AssetMetadata metadata);

    /** @brief Removes metadata by stable identifier. */
    bool unregisterAsset(AssetId id);

    /** @brief Reports whether an identifier is registered. */
    [[nodiscard]] bool contains(AssetId id) const noexcept;

    /** @brief Reports whether a normalized source path is registered. */
    [[nodiscard]] bool contains(const std::filesystem::path& path) const;

    /** @brief Returns copied metadata for an identifier, or no value if unknown. */
    [[nodiscard]] std::optional<AssetMetadata> find(AssetId id) const;

    /** @brief Returns copied metadata for a source path, or no value if unknown. */
    [[nodiscard]] std::optional<AssetMetadata> find(
        const std::filesystem::path& path
    ) const;

    /** @brief Updates the source path while preserving the stable asset ID. */
    void updatePath(AssetId id, std::filesystem::path path);

    /** @brief Writes the stable ID-to-path catalog as deterministic JSON. */
    void save(const std::filesystem::path& path) const;

    /** @brief Replaces the catalog from a previously saved JSON file. */
    void load(const std::filesystem::path& path);

    /** @brief Removes all registered metadata. */
    void clear() noexcept;

    /** @brief Returns the number of known project assets. */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    [[nodiscard]] static std::string pathKey(const std::filesystem::path& path);

    std::unordered_map<AssetId, AssetMetadata> m_assetsById;
    std::unordered_map<std::string, AssetId> m_idsByPath;
};

} // namespace vshade::asset
