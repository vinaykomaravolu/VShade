#pragma once

#include "asset/Asset.hpp"
#include "asset/AssetHandle.hpp"
#include "asset/AssetLoader.hpp"
#include "asset/AssetRef.hpp"
#include "asset/AssetRegistry.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace vshade::asset {

/** @brief Loads, owns, and reuses typed assets identified by normalized paths. */
class AssetManager final {
public:
    /**
     * @brief Creates a manager with the engine's built-in loaders installed.
     *
     * AudioClip, Texture2D, Shader, Mesh, Model, and Scene assets can be loaded immediately.
     * Registering another loader for one of those types replaces its default.
     */
    AssetManager();
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) noexcept = default;
    AssetManager& operator=(AssetManager&&) noexcept = default;

    /**
     * @brief Registers or replaces the loader used for @p Resource.
     * @param loader Callable receiving a normalized path and returning a shared resource.
     */
    template<typename Resource, typename Loader>
    void registerLoader(Loader&& loader) {
        static_assert(!std::is_void_v<Resource>, "An asset resource cannot be void");
        std::function<std::shared_ptr<Resource>(const std::filesystem::path&)> typedLoader(
            std::forward<Loader>(loader)
        );
        if (!typedLoader) {
            throw std::invalid_argument("An asset loader cannot be empty");
        }

        registerLoaderErased(
            typeid(Resource),
            [typedLoader = std::move(typedLoader)](const std::filesystem::path& path) {
                return std::static_pointer_cast<void>(typedLoader(path));
            }
        );
    }

    /** @brief Registers an owned typed loader implementation. */
    template<typename Resource>
    void registerAssetLoader(std::shared_ptr<AssetLoader<Resource>> loader) {
        if (!loader) {
            throw std::invalid_argument("An asset loader cannot be null");
        }
        registerLoader<Resource>(
            [loader = std::move(loader)](const std::filesystem::path& path) {
                return loader->load(path);
            }
        );
    }

    /**
     * @brief Catalogs a typed asset without loading its runtime resource.
     * @return Handle containing the caller-provided stable identifier.
     */
    template<typename Resource>
    [[nodiscard]] AssetHandle<Resource> registerAsset(
        const AssetId id,
        const std::filesystem::path& path
    ) {
        registerAssetErased({.id = id, .sourcePath = path});
        return AssetHandle<Resource>(id);
    }

    /**
     * @brief Loads an asset or returns its existing cached handle.
     * @throws std::invalid_argument If the path is empty or no loader is registered.
     * @throws std::runtime_error If the loader returns a null resource.
     * @throws std::logic_error If the path identifier collides with another asset.
     */
    template<typename Resource>
    [[nodiscard]] AssetHandle<Resource> load(const std::filesystem::path& path) {
        return AssetHandle<Resource>(loadErased(typeid(Resource), path));
    }

    /** @brief Catalogs a path and returns a reference that can resolve in a fresh manager. */
    template<typename Resource>
    [[nodiscard]] AssetReference<Resource> reference(
        const std::filesystem::path& path
    ) {
        const AssetMetadata metadata = referenceErased(typeid(Resource), path);
        return {AssetHandle<Resource>::fromId(metadata.id), metadata.sourcePath};
    }

    /** @brief Loads and returns an owning resource view in one operation. */
    template<typename Resource>
    [[nodiscard]] AssetRef<Resource> loadResource(
        const std::filesystem::path& path
    ) {
        const AssetReference<Resource> assetReference = reference<Resource>(path);
        load(assetReference.handle());
        return {assetReference, get(assetReference.handle())};
    }


    /** @brief Resolves a serialized reference, registering its source when needed. */
    template<typename Resource>
    [[nodiscard]] AssetRef<Resource> loadResource(
        const AssetReference<Resource>& assetReference
    ) {
        if (!assetReference.valid()) {
            throw std::invalid_argument("Cannot load an invalid asset reference");
        }
        registerReferenceErased(
            typeid(Resource),
            {assetReference.handle().id(), assetReference.sourcePath()}
        );
        load(assetReference.handle());
        return {assetReference, get(assetReference.handle())};
    }

    /** @brief Loads the resource for a previously registered handle. */
    template<typename Resource>
    void load(const AssetHandle<Resource> handle) {
        loadErased(typeid(Resource), handle.id());
    }

    /** @brief Returns the shared resource, or null if the handle is not loaded. */
    template<typename Resource>
    [[nodiscard]] std::shared_ptr<Resource> get(const AssetHandle<Resource> handle) const {
        return std::static_pointer_cast<Resource>(getErased(handle.id(), typeid(Resource)));
    }

    /** @brief Reports whether a compatible resource is loaded for the handle. */
    template<typename Resource>
    [[nodiscard]] bool isLoaded(const AssetHandle<Resource> handle) const noexcept {
        return isLoadedErased(handle.id(), typeid(Resource));
    }

    /** @brief Removes a resource from the cache and reports whether it existed. */
    template<typename Resource>
    bool unload(const AssetHandle<Resource> handle) {
        return unloadErased(handle.id(), typeid(Resource));
    }

    /** @brief Returns copied catalog metadata for a registered handle. */
    template<typename Resource>
    [[nodiscard]] std::optional<AssetMetadata> metadata(
        const AssetHandle<Resource> handle
    ) const {
        return metadataErased(handle.id(), typeid(Resource));
    }

    /**
     * @brief Removes a registered asset and any resource currently cached for it.
     */
    template<typename Resource>
    bool unregisterAsset(const AssetHandle<Resource> handle) {
        return unregisterAssetErased(handle.id(), typeid(Resource));
    }

    /** @brief Removes every resource while external shared owners remain valid. */
    void clear() noexcept;

    /** @brief Writes the persistent asset ID-to-path catalog. */
    void saveCatalog(const std::filesystem::path& path) const;

    /** @brief Loads catalog entries used to resolve serialized handles. */
    void loadCatalog(const std::filesystem::path& path);

    /** @brief Returns the number of resources currently cached. */
    [[nodiscard]] std::size_t size() const noexcept;

    /** @brief Returns the number of assets known to the internal registry. */
    [[nodiscard]] std::size_t registeredAssetCount() const noexcept;

private:
    using ErasedLoader =
        std::function<std::shared_ptr<void>(const std::filesystem::path&)>;

    struct Record {
        AssetMetadata metadata;
        std::type_index type{typeid(void)};
        std::shared_ptr<void> resource;
    };

    void registerLoaderErased(std::type_index type, ErasedLoader loader);
    void registerAssetErased(AssetMetadata metadata);
    [[nodiscard]] AssetMetadata referenceErased(
        std::type_index type,
        const std::filesystem::path& path
    );
    void registerReferenceErased(std::type_index type, AssetMetadata metadata);
    [[nodiscard]] AssetId loadErased(
        std::type_index type,
        const std::filesystem::path& path
    );
    void loadErased(std::type_index type, AssetId id);
    [[nodiscard]] std::shared_ptr<void> getErased(
        AssetId id,
        std::type_index type
    ) const;
    [[nodiscard]] bool isLoadedErased(AssetId id, std::type_index type) const noexcept;
    bool unloadErased(AssetId id, std::type_index type);
    [[nodiscard]] std::optional<AssetMetadata> metadataErased(
        AssetId id,
        std::type_index type
    ) const;
    bool unregisterAssetErased(AssetId id, std::type_index type);

    std::unordered_map<std::type_index, ErasedLoader> m_loaders;
    std::unordered_map<AssetId, Record> m_assets;
    AssetRegistry m_registry;
};

} // namespace vshade::asset
