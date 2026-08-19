#include "asset/Asset.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace vshade::asset {
namespace {

[[nodiscard]] std::string lowercase(std::string value) {
    std::ranges::transform(
        value,
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );
    return value;
}

} // namespace

const char* toString(const AssetType type) noexcept {
    switch (type) {
        case AssetType::Texture:
            return "Texture";
        case AssetType::Model:
            return "Model";
        case AssetType::Audio:
            return "Audio";
        case AssetType::Scene:
            return "Scene";
        case AssetType::Prefab:
            return "Prefab";
        case AssetType::Shader:
            return "Shader";
        case AssetType::Mesh:
            return "Mesh";
        case AssetType::Unknown:
            return "Unknown";
    }
    return "Unknown";
}

std::optional<AssetType> assetTypeFromString(const std::string_view name) {
    const std::string normalized = lowercase(std::string(name));
    if (normalized == "texture") {
        return AssetType::Texture;
    }
    if (normalized == "model") {
        return AssetType::Model;
    }
    if (normalized == "audio") {
        return AssetType::Audio;
    }
    if (normalized == "scene") {
        return AssetType::Scene;
    }
    if (normalized == "prefab") {
        return AssetType::Prefab;
    }
    if (normalized == "shader") {
        return AssetType::Shader;
    }
    if (normalized == "mesh") {
        return AssetType::Mesh;
    }
    if (normalized == "unknown") {
        return AssetType::Unknown;
    }
    return std::nullopt;
}

AssetType assetTypeFromExtension(const std::filesystem::path& extension) {
    const std::string value = lowercase(extension.generic_string());
    if (value == ".png"
        || value == ".jpg"
        || value == ".jpeg"
        || value == ".bmp"
        || value == ".tga") {
        return AssetType::Texture;
    }
    if (value == ".glb" || value == ".gltf") {
        return AssetType::Model;
    }
    if (value == ".wav"
        || value == ".mp3"
        || value == ".flac"
        || value == ".ogg") {
        return AssetType::Audio;
    }
    if (value == ".vscene") {
        return AssetType::Scene;
    }
    if (value == ".vsprefab") {
        return AssetType::Prefab;
    }
    if (value == ".vert"
        || value == ".frag"
        || value == ".glsl"
        || value == ".shader") {
        return AssetType::Shader;
    }
    return AssetType::Unknown;
}

} // namespace vshade::asset
