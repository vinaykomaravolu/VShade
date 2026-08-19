#include "asset/AssetManager.hpp"

#include "asset/loader/AudioLoader.hpp"
#include "asset/loader/MeshLoader.hpp"
#include "asset/loader/ModelLoader.hpp"
#include "asset/loader/PrefabLoader.hpp"
#include "asset/loader/SceneLoader.hpp"
#include "asset/loader/ShaderLoader.hpp"
#include "asset/loader/TextureLoader.hpp"
#include "audio/AudioClip.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Model.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Texture.hpp"
#include "scene/Scene.hpp"
#include "scene/Prefab.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace vshade::asset {
namespace {

[[nodiscard]] std::filesystem::path normalizedAssetPath(
    const std::filesystem::path& path
) {
    if (path.empty()) {
        throw std::invalid_argument("An asset path cannot be empty");
    }
    return path.lexically_normal();
}

[[nodiscard]] AssetId assetIdForPath(const std::filesystem::path& path) {
    // FNV-1a keeps path-derived IDs deterministic across processes and platforms.
    constexpr AssetId offsetBasis = 14695981039346656037ULL;
    constexpr AssetId prime = 1099511628211ULL;
    AssetId id = offsetBasis;
    for (const unsigned char character : path.generic_string()) {
        id ^= character;
        id *= prime;
    }
    return id == invalidAssetId ? 1 : id;
}

} // namespace

AssetManager::AssetManager() {
    registerAssetLoader<audio::AudioClip>(std::make_shared<AudioLoader>());
    registerAssetLoader<renderer::Texture2D>(std::make_shared<TextureLoader>());
    registerAssetLoader<renderer::Shader>(std::make_shared<ShaderLoader>());
    registerAssetLoader<renderer::Mesh>(std::make_shared<MeshLoader>());
    registerAssetLoader<renderer::Model>(std::make_shared<ModelLoader>());
    registerAssetLoader<scene::Scene>(std::make_shared<SceneLoader>());
    registerAssetLoader<scene::Prefab>(std::make_shared<PrefabLoader>());
}

void AssetManager::registerLoaderErased(
    const std::type_index type,
    ErasedLoader loader
) {
    m_loaders.insert_or_assign(type, std::move(loader));
}

void AssetManager::registerAssetErased(
    AssetMetadata metadata,
    const std::type_index type
) {
    const AssetId id = metadata.id;
    m_registry.registerAsset(std::move(metadata));
    m_assetTypes.insert_or_assign(id, type);
}

AssetMetadata AssetManager::referenceErased(
    const std::type_index type,
    const std::filesystem::path& path
) {
    const std::filesystem::path normalizedPath = normalizedAssetPath(path);
    if (const std::optional<AssetMetadata> registered = m_registry.find(normalizedPath)) {
        if (const auto knownType = m_assetTypes.find(registered->id);
            knownType != m_assetTypes.end() && knownType->second != type) {
            throw std::logic_error("An asset path is already used by another resource type");
        }
        m_assetTypes.insert_or_assign(registered->id, type);
        return *registered;
    }

    AssetMetadata metadata{
        .id = assetIdForPath(normalizedPath),
        .sourcePath = normalizedPath,
    };
    m_registry.registerAsset(metadata);
    m_assetTypes.insert_or_assign(metadata.id, type);
    return metadata;
}

void AssetManager::registerReferenceErased(
    const std::type_index type,
    AssetMetadata metadata
) {
    const std::filesystem::path normalizedPath = normalizedAssetPath(metadata.sourcePath);
    const AssetId expectedId = assetIdForPath(normalizedPath);
    if (metadata.id != expectedId) {
        throw std::invalid_argument("Asset reference ID does not match its normalized path");
    }
    if (const auto existing = m_registry.find(metadata.id)) {
        if (existing->sourcePath != normalizedPath) {
            throw std::logic_error("Asset reference ID collides with another path");
        }
    } else {
        metadata.sourcePath = normalizedPath;
        m_registry.registerAsset(std::move(metadata));
    }
    if (const auto loaded = m_assets.find(expectedId);
        loaded != m_assets.end() && loaded->second.type != type) {
        throw std::logic_error("An asset ID is already loaded as another type");
    }
    if (const auto knownType = m_assetTypes.find(expectedId);
        knownType != m_assetTypes.end() && knownType->second != type) {
        throw std::logic_error("An asset ID is already used by another resource type");
    }
    m_assetTypes.insert_or_assign(expectedId, type);
}

AssetId AssetManager::loadErased(
    const std::type_index type,
    const std::filesystem::path& path
) {
    const AssetMetadata metadata = referenceErased(type, path);
    loadErased(type, metadata.id);
    return metadata.id;
}

void AssetManager::loadErased(const std::type_index type, const AssetId id) {
    if (id == invalidAssetId) {
        throw std::invalid_argument("An invalid asset handle cannot be loaded");
    }

    const std::optional<AssetMetadata> metadata = m_registry.find(id);
    if (!metadata.has_value()) {
        throw std::invalid_argument("The asset handle is not registered");
    }
    if (const auto knownType = m_assetTypes.find(id);
        knownType != m_assetTypes.end() && knownType->second != type) {
        throw std::logic_error("The asset handle type does not match the registered resource");
    }
    m_assetTypes.insert_or_assign(id, type);

    if (const auto existing = m_assets.find(id); existing != m_assets.end()) {
        if (existing->second.type != type) {
            throw std::logic_error("An asset ID is already loaded as another type");
        }
        return;
    }

    const auto loader = m_loaders.find(type);
    if (loader == m_loaders.end()) {
        throw std::invalid_argument("No loader is registered for the requested asset type");
    }

    std::shared_ptr<void> resource = loader->second(metadata->sourcePath);
    if (!resource) {
        throw std::runtime_error("The asset loader returned a null resource");
    }

    m_assets.emplace(
        id,
        Record{
            .metadata = *metadata,
            .type = type,
            .resource = std::move(resource),
        }
    );
}

std::shared_ptr<void> AssetManager::getErased(
    const AssetId id,
    const std::type_index type
) const {
    const auto asset = m_assets.find(id);
    if (asset == m_assets.end()) {
        return nullptr;
    }
    if (asset->second.type != type) {
        throw std::logic_error("The asset handle type does not match the cached resource");
    }
    return asset->second.resource;
}

bool AssetManager::isLoadedErased(
    const AssetId id,
    const std::type_index type
) const noexcept {
    const auto asset = m_assets.find(id);
    return asset != m_assets.end() && asset->second.type == type;
}

bool AssetManager::unloadErased(const AssetId id, const std::type_index type) {
    const auto asset = m_assets.find(id);
    if (asset == m_assets.end()) {
        return false;
    }
    if (asset->second.type != type) {
        throw std::logic_error("The asset handle type does not match the cached resource");
    }
    m_assets.erase(asset);
    return true;
}

std::optional<AssetMetadata> AssetManager::metadataErased(
    const AssetId id,
    const std::type_index type
) const {
    const auto knownType = m_assetTypes.find(id);
    if (knownType != m_assetTypes.end() && knownType->second != type) {
        throw std::logic_error("The asset handle type does not match the registered resource");
    }
    return m_registry.find(id);
}

std::vector<AssetMetadata> AssetManager::loadedAssetsErased(
    const std::type_index type
) const {
    std::vector<AssetMetadata> assets;
    for (const auto& [id, record] : m_assets) {
        static_cast<void>(id);
        if (record.type == type) {
            assets.push_back(record.metadata);
        }
    }
    std::ranges::sort(
        assets,
        {},
        [](const AssetMetadata& metadata) {
            return metadata.sourcePath.generic_string();
        }
    );
    return assets;
}

std::vector<AssetMetadata> AssetManager::knownAssetsErased(
    const std::type_index type
) const {
    std::vector<AssetMetadata> assets;
    for (const auto& [id, knownType] : m_assetTypes) {
        if (knownType == type) {
            if (const auto metadata = m_registry.find(id)) {
                assets.push_back(*metadata);
            }
        }
    }
    std::ranges::sort(
        assets,
        {},
        [](const AssetMetadata& metadata) {
            return metadata.sourcePath.generic_string();
        }
    );
    return assets;
}

bool AssetManager::unregisterAssetErased(
    const AssetId id,
    const std::type_index type
) {
    const auto knownType = m_assetTypes.find(id);
    if (knownType != m_assetTypes.end() && knownType->second != type) {
        throw std::logic_error("The asset handle type does not match the registered resource");
    }
    const auto asset = m_assets.find(id);
    if (asset != m_assets.end()) {
        if (asset->second.type != type) {
            throw std::logic_error("The asset handle type does not match the cached resource");
        }
        m_assets.erase(asset);
    }
    m_assetTypes.erase(id);
    return m_registry.unregisterAsset(id);
}

void AssetManager::clear() noexcept {
    m_assets.clear();
}

void AssetManager::saveCatalog(const std::filesystem::path& path) const {
    m_registry.save(path);
}

void AssetManager::loadCatalog(const std::filesystem::path& path) {
    if (!m_assets.empty()) {
        throw std::logic_error("Cannot replace the asset catalog while resources are loaded");
    }
    m_registry.load(path);
    m_assetTypes.clear();
}

std::size_t AssetManager::size() const noexcept {
    return m_assets.size();
}

std::size_t AssetManager::registeredAssetCount() const noexcept {
    return m_registry.size();
}

} // namespace vshade::asset
