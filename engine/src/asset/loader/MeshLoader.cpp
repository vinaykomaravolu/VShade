#include "asset/loader/MeshLoader.hpp"

#include "asset/loader/GltfLoaderUtils.hpp"

#include <stdexcept>

namespace vshade::asset {

std::shared_ptr<renderer::Mesh> MeshLoader::load(
    const std::filesystem::path& path
) const {
    const fastgltf::Asset asset = detail::loadGltfAsset(path);
    if (asset.meshes.empty() || asset.meshes.front().primitives.empty()) {
        throw std::invalid_argument("A glTF mesh asset requires at least one primitive");
    }
    return detail::loadGltfPrimitive(asset, asset.meshes.front().primitives.front());
}

} // namespace vshade::asset
