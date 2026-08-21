#include "EditorIcons.hpp"

#include <core/Log.hpp>
#include <renderer/Texture.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace editor {
namespace {

std::shared_ptr<vshade::renderer::Texture2D> g_folder;
std::shared_ptr<vshade::renderer::Texture2D> g_file;
std::shared_ptr<vshade::renderer::Texture2D> g_texture;
std::shared_ptr<vshade::renderer::Texture2D> g_model;
std::shared_ptr<vshade::renderer::Texture2D> g_audio;
std::shared_ptr<vshade::renderer::Texture2D> g_scene;
std::shared_ptr<vshade::renderer::Texture2D> g_prefab;
std::shared_ptr<vshade::renderer::Texture2D> g_camera;
std::shared_ptr<vshade::renderer::Texture2D> g_directionalLight;
std::shared_ptr<vshade::renderer::Texture2D> g_pointLight;
std::shared_ptr<vshade::renderer::Texture2D> g_speaker;

[[nodiscard]] std::filesystem::path iconDirectory() {
#ifdef VSHADE_EDITOR_ICON_DIR
    return std::filesystem::path{VSHADE_EDITOR_ICON_DIR};
#else
    return std::filesystem::path{"icons"};
#endif
}

[[nodiscard]] std::shared_ptr<vshade::renderer::Texture2D> loadIcon(
    const char* filename
) {
    const std::filesystem::path path = iconDirectory() / filename;
    try {
        return std::make_shared<vshade::renderer::Texture2D>(
            vshade::renderer::Texture2D::fromFile(
                path,
                vshade::renderer::TextureFilter::Linear,
                vshade::renderer::TextureWrap::ClampToEdge,
                false
            )
        );
    } catch (const std::exception& error) {
        ENGINE_ERROR(
            "Failed to load editor icon '{}': {}; using fallback",
            path.generic_string(),
            error.what()
        );
        constexpr std::array<std::uint8_t, 16> fallbackPixels{
            255, 0, 255, 255,
            32, 32, 32, 255,
            32, 32, 32, 255,
            255, 0, 255, 255,
        };
        return std::make_shared<vshade::renderer::Texture2D>(
            2,
            2,
            vshade::renderer::TextureFormat::RGBA8,
            fallbackPixels.data(),
            vshade::renderer::TextureFilter::Nearest,
            vshade::renderer::TextureWrap::ClampToEdge
        );
    }
}

[[nodiscard]] std::shared_ptr<vshade::renderer::Texture2D> loadOptionalIcon(
    const char* filename
) {
    const std::filesystem::path path = iconDirectory() / filename;
    try {
        return std::make_shared<vshade::renderer::Texture2D>(
            vshade::renderer::Texture2D::fromFile(
                path,
                vshade::renderer::TextureFilter::Linear,
                vshade::renderer::TextureWrap::ClampToEdge,
                false
            )
        );
    } catch (const std::exception& error) {
        ENGINE_WARN(
            "Failed to load optional editor icon '{}': {}; using vector fallback",
            path.generic_string(),
            error.what()
        );
        return {};
    }
}

} // namespace

void EditorIcons::initialize() {
    shutdown();
    g_folder = loadIcon("folder.png");
    g_file = loadIcon("file.png");
    g_texture = loadIcon("texture.png");
    g_model = loadIcon("model.png");
    g_audio = loadIcon("audio.png");
    g_scene = loadIcon("scene.png");
    g_prefab = loadIcon("prefab.png");
    g_camera = loadOptionalIcon("camera.png");
    g_directionalLight = loadOptionalIcon("directional_light.png");
    g_pointLight = loadOptionalIcon("point_light.png");
    g_speaker = loadOptionalIcon("speaker.png");
}

void EditorIcons::shutdown() {
    g_folder.reset();
    g_file.reset();
    g_texture.reset();
    g_model.reset();
    g_audio.reset();
    g_scene.reset();
    g_prefab.reset();
    g_camera.reset();
    g_directionalLight.reset();
    g_pointLight.reset();
    g_speaker.reset();
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::folder() {
    return g_folder;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::file() {
    return g_file;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::texture() {
    return g_texture;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::model() {
    return g_model;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::audio() {
    return g_audio;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::scene() {
    return g_scene;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::prefab() {
    return g_prefab;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::camera() {
    return g_camera;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::directionalLight() {
    return g_directionalLight;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::pointLight() {
    return g_pointLight;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::speaker() {
    return g_speaker;
}

std::shared_ptr<vshade::renderer::Texture2D> EditorIcons::forAsset(
    const vshade::asset::AssetType type
) {
    switch (type) {
        case vshade::asset::AssetType::Texture:
            return texture();
        case vshade::asset::AssetType::Model:
            return model();
        case vshade::asset::AssetType::Audio:
            return audio();
        case vshade::asset::AssetType::Scene:
            return scene();
        case vshade::asset::AssetType::Prefab:
            return prefab();
        default:
            return file();
    }
}

} // namespace editor
