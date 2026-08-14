#include "asset/AssetManager.hpp"

#include "asset/loader/MeshLoader.hpp"
#include "asset/loader/ModelLoader.hpp"
#include "asset/loader/SceneLoader.hpp"
#include "asset/loader/ShaderLoader.hpp"
#include "asset/loader/TextureLoader.hpp"
#include "renderer/mesh.hpp"
#include "renderer/model.hpp"
#include "renderer/shader.hpp"
#include "renderer/texture.hpp"
#include "scene/Scene.hpp"

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
    registerAssetLoader<renderer::Texture2D>(std::make_shared<TextureLoader>());
    registerAssetLoader<renderer::Shader>(std::make_shared<ShaderLoader>());
    registerAssetLoader<renderer::Mesh>(std::make_shared<MeshLoader>());
    registerAssetLoader<renderer::Model>(std::make_shared<ModelLoader>());
    registerAssetLoader<scene::Scene>(std::make_shared<SceneLoader>());
}

void AssetManager::registerLoaderErased(
    const std::type_index type,
    ErasedLoader loader
) {
    m_loaders.insert_or_assign(type, std::move(loader));
}

void AssetManager::registerAssetErased(AssetMetadata metadata) {
    m_registry.registerAsset(std::move(metadata));
}

AssetId AssetManager::loadErased(
    const std::type_index type,
    const std::filesystem::path& path
) {
    const std::filesystem::path normalizedPath = normalizedAssetPath(path);
    const std::optional<AssetMetadata> registered = m_registry.find(normalizedPath);
    const AssetId id = registered.has_value()
        ? registered->id
        : assetIdForPath(normalizedPath);

    if (!registered.has_value()) {
        m_registry.registerAsset({.id = id, .sourcePath = normalizedPath});
    }

    loadErased(type, id);
    return id;
}

void AssetManager::loadErased(const std::type_index type, const AssetId id) {
    if (id == invalidAssetId) {
        throw std::invalid_argument("An invalid asset handle cannot be loaded");
    }

    const std::optional<AssetMetadata> metadata = m_registry.find(id);
    if (!metadata.has_value()) {
        throw std::invalid_argument("The asset handle is not registered");
    }

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
    const auto asset = m_assets.find(id);
    if (asset != m_assets.end() && asset->second.type != type) {
        throw std::logic_error("The asset handle type does not match the cached resource");
    }
    return m_registry.find(id);
}

bool AssetManager::unregisterAssetErased(
    const AssetId id,
    const std::type_index type
) {
    const auto asset = m_assets.find(id);
    if (asset != m_assets.end()) {
        if (asset->second.type != type) {
            throw std::logic_error("The asset handle type does not match the cached resource");
        }
        m_assets.erase(asset);
    }
    return m_registry.unregisterAsset(id);
}

void AssetManager::clear() noexcept {
    m_assets.clear();
}

std::size_t AssetManager::size() const noexcept {
    return m_assets.size();
}

std::size_t AssetManager::registeredAssetCount() const noexcept {
    return m_registry.size();
}

} // namespace vshade::asset
