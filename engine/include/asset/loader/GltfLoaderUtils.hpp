#pragma once

#include <fastgltf/types.hpp>

#include <filesystem>
#include <memory>

namespace vshade::renderer {
class Mesh;
}

namespace vshade::asset::detail {

[[nodiscard]] fastgltf::Asset loadGltfAsset(const std::filesystem::path& path);

[[nodiscard]] std::shared_ptr<renderer::Mesh> loadGltfPrimitive(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive
);

} // namespace vshade::asset::detail
