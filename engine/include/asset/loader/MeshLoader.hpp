#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::renderer {
class Mesh;
}

namespace vshade::asset {

/**
 * @brief Loads glTF 2.0 mesh files into renderer-ready geometry resources.
 *
 * The current renderer Mesh type represents one draw primitive, so this loader
 * imports the first primitive of the first mesh in a .gltf or .glb asset.
 */
class MeshLoader final : public AssetLoader<renderer::Mesh> {
public:
    MeshLoader() = default;
    ~MeshLoader() override = default;

    /**
     * @brief Reads the first glTF mesh primitive and creates its runtime geometry.
     * @throws std::runtime_error If the file cannot be read or parsed.
     * @throws std::invalid_argument If the asset has no supported mesh primitive.
     */
    [[nodiscard]] std::shared_ptr<renderer::Mesh> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
