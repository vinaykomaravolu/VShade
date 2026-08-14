#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::renderer {
class Model;
}

namespace vshade::asset {

/**
 * @brief Imports complete glTF mesh primitives, materials, and node hierarchy.
 *
 * Initial material import includes base-color textures and factors, roughness,
 * metallic, and unlit properties. Normal, metallic-roughness, occlusion, and
 * emissive textures, plus skins, morph targets, and animations, remain future
 * Model extensions rather than being silently flattened into Mesh.
 */
class ModelLoader final : public AssetLoader<renderer::Model> {
public:
    ModelLoader() = default;
    ~ModelLoader() override = default;

    /** @brief Loads a .gltf or .glb file as a complete renderer model. */
    [[nodiscard]] std::shared_ptr<renderer::Model> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
