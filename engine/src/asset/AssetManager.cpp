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
#include <optional>
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

[[nodiscard]] AssetType assetTypeFor(const std::type_index type) {
    if (type == typeid(renderer::Texture2D)) {
        return AssetType::Texture;
    }
    if (type == typeid(renderer::Model)) {
        return AssetType::Model;
    }
    if (type == typeid(audio::AudioClip)) {
        return AssetType::Audio;
    }
    if (type == typeid(scene::Scene)) {
        return AssetType::Scene;
    }
    if (type == typeid(scene::Prefab)) {
        return AssetType::Prefab;
    }
    if (type == typeid(renderer::Shader)) {
        return AssetType::Shader;
    }
    if (type == typeid(renderer::Mesh)) {
        return AssetType::Mesh;
    }
    return AssetType::Unknown;
}

[[nodiscard]] std::optional<std::type_index> typeIndexFor(
    const AssetType type
) {
    switch (type) {
        case AssetType::Texture:
            return typeid(renderer::Texture2D);
        case AssetType::Model:
            return typeid(renderer::Model);
        case AssetType::Audio:
            return typeid(audio::AudioClip);
        case AssetType::Scene:
            return typeid(scene::Scene);
        case AssetType::Prefab:
            return typeid(scene::Prefab);
        case AssetType::Shader:
            return typeid(renderer::Shader);
        case AssetType::Mesh:
            return typeid(renderer::Mesh);
        case AssetType::Unknown:
            return std::nullopt;
    }
    return std::nullopt;
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
    metadata.sourcePath = toCatalogPath(metadata.sourcePath);
    if (metadata.type == AssetType::Unknown) {
        metadata.type = assetTypeFor(type);
    }
    const AssetId id = metadata.id;
    m_registry.registerAsset(std::move(metadata));
    rememberType(id, type);
}

AssetMetadata AssetManager::referenceErased(
    const std::type_index type,
    const std::filesystem::path& path
) {
    const std::filesystem::path catalogPath = toCatalogPath(path);
    const AssetType catalogType = assetTypeFor(type);
    if (const std::optional<AssetMetadata> registered =
            m_registry.find(catalogPath)) {
        if (const auto knownType = m_assetTypes.find(registered->id);
            knownType != m_assetTypes.end() && knownType->second != type) {
            throw std::logic_error(
                "An asset path is already used by another resource type"
            );
        }
        if (catalogType != AssetType::Unknown) {
            m_registry.setType(registered->id, catalogType);
        }
        rememberType(registered->id, type);
        auto metadata = *registered;
        metadata.type = catalogType == AssetType::Unknown
            ? metadata.type
            : catalogType;
        return metadata;
    }

    AssetMetadata metadata{
        .id = assetIdForPath(catalogPath),
        .sourcePath = catalogPath,
        .type = catalogType,
    };
    m_registry.registerAsset(metadata);
    rememberType(metadata.id, type);
    return metadata;
}

void AssetManager::registerReferenceErased(
    const std::type_index type,
    AssetMetadata metadata
) {
    const std::filesystem::path catalogPath = toCatalogPath(metadata.sourcePath);
    const AssetId expectedId = assetIdForPath(catalogPath);
    if (metadata.id != expectedId) {
        throw std::invalid_argument("Asset reference ID does not match its normalized path");
    }
    if (const auto existing = m_registry.find(metadata.id)) {
        if (existing->sourcePath != catalogPath) {
            throw std::logic_error("Asset reference ID collides with another path");
        }
    } else if (const auto existingPath = m_registry.find(catalogPath)) {
        if (existingPath->id != metadata.id) {
            throw std::logic_error("An asset path is already registered");
        }
    } else {
        metadata.sourcePath = catalogPath;
        metadata.type = assetTypeFor(type);
        m_registry.registerAsset(metadata);
    }
    if (const auto loaded = m_assets.find(expectedId);
        loaded != m_assets.end() && loaded->second.type != type) {
        throw std::logic_error("An asset ID is already loaded as another type");
    }
    if (const auto knownType = m_assetTypes.find(expectedId);
        knownType != m_assetTypes.end() && knownType->second != type) {
        throw std::logic_error("An asset ID is already used by another resource type");
    }
    if (const auto catalogType = assetTypeFor(type);
        catalogType != AssetType::Unknown) {
        m_registry.setType(expectedId, catalogType);
    }
    rememberType(expectedId, type);
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

    std::shared_ptr<void> resource = loader->second(
        resolveSourcePath(metadata->sourcePath)
    );
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

void AssetManager::clearCatalog() noexcept {
    m_assets.clear();
    m_assetTypes.clear();
    m_registry.clear();
}

void AssetManager::setRootDirectory(std::filesystem::path directory) {
    if (directory.empty()) {
        m_rootDirectory.clear();
        return;
    }
    m_rootDirectory = std::filesystem::absolute(directory).lexically_normal();
}

const std::filesystem::path& AssetManager::rootDirectory() const noexcept {
    return m_rootDirectory;
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
    for (const AssetMetadata& metadata : m_registry.all()) {
        if (const auto type = typeIndexFor(metadata.type)) {
            m_assetTypes.insert_or_assign(metadata.id, *type);
        }
    }
}

std::filesystem::path AssetManager::toCatalogPath(
    const std::filesystem::path& path
) const {
    const std::filesystem::path normalized = normalizedAssetPath(path);
    if (m_rootDirectory.empty() || normalized.is_relative()) {
        return std::filesystem::path(normalized.generic_string());
    }

    const std::filesystem::path relative =
        normalized.lexically_relative(m_rootDirectory);
    if (relative.empty() || *relative.begin() == "..") {
        return std::filesystem::path(normalized.generic_string());
    }
    return std::filesystem::path(relative.generic_string());
}

std::filesystem::path AssetManager::resolveSourcePath(
    const std::filesystem::path& catalogPath
) const {
    if (catalogPath.is_absolute() || m_rootDirectory.empty()) {
        return catalogPath;
    }
    return (m_rootDirectory / catalogPath).lexically_normal();
}

void AssetManager::rememberType(const AssetId id, const std::type_index type) {
    m_assetTypes.insert_or_assign(id, type);
}

std::size_t AssetManager::size() const noexcept {
    return m_assets.size();
}

std::size_t AssetManager::registeredAssetCount() const noexcept {
    return m_registry.size();
}

} // namespace vshade::asset
