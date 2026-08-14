#include "asset/AssetRegistry.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::asset {

std::string AssetRegistry::pathKey(const std::filesystem::path& path) {
    if (path.empty()) {
        throw std::invalid_argument("An asset path cannot be empty");
    }
    return path.lexically_normal().generic_string();
}

void AssetRegistry::registerAsset(AssetMetadata metadata) {
    if (metadata.id == invalidAssetId) {
        throw std::invalid_argument("A registered asset ID cannot be zero");
    }

    const std::string key = pathKey(metadata.sourcePath);
    if (m_assetsById.contains(metadata.id)) {
        throw std::logic_error("The asset ID is already registered");
    }
    if (m_idsByPath.contains(key)) {
        throw std::logic_error("The asset path is already registered");
    }

    metadata.sourcePath = std::filesystem::path(key);
    const AssetId id = metadata.id;
    m_assetsById.emplace(id, std::move(metadata));
    m_idsByPath.emplace(key, id);
}

bool AssetRegistry::unregisterAsset(const AssetId id) {
    const auto asset = m_assetsById.find(id);
    if (asset == m_assetsById.end()) {
        return false;
    }

    m_idsByPath.erase(asset->second.sourcePath.generic_string());
    m_assetsById.erase(asset);
    return true;
}

bool AssetRegistry::contains(const AssetId id) const noexcept {
    return m_assetsById.contains(id);
}

bool AssetRegistry::contains(const std::filesystem::path& path) const {
    return !path.empty() && m_idsByPath.contains(pathKey(path));
}

std::optional<AssetMetadata> AssetRegistry::find(const AssetId id) const {
    const auto asset = m_assetsById.find(id);
    if (asset == m_assetsById.end()) {
        return std::nullopt;
    }
    return asset->second;
}

std::optional<AssetMetadata> AssetRegistry::find(
    const std::filesystem::path& path
) const {
    if (path.empty()) {
        return std::nullopt;
    }
    const auto id = m_idsByPath.find(pathKey(path));
    return id == m_idsByPath.end() ? std::nullopt : find(id->second);
}

void AssetRegistry::updatePath(
    const AssetId id,
    std::filesystem::path path
) {
    const auto asset = m_assetsById.find(id);
    if (asset == m_assetsById.end()) {
        throw std::out_of_range("The asset ID is not registered");
    }

    const std::string newKey = pathKey(path);
    if (const auto existing = m_idsByPath.find(newKey);
        existing != m_idsByPath.end() && existing->second != id) {
        throw std::logic_error("The asset path is already registered");
    }

    m_idsByPath.erase(asset->second.sourcePath.generic_string());
    asset->second.sourcePath = std::filesystem::path(newKey);
    m_idsByPath.insert_or_assign(newKey, id);
}

void AssetRegistry::clear() noexcept {
    m_assetsById.clear();
    m_idsByPath.clear();
}

std::size_t AssetRegistry::size() const noexcept {
    return m_assetsById.size();
}

} // namespace vshade::asset
