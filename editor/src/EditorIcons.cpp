#include "EditorIcons.hpp"

#include <renderer/Texture.hpp>

#include <filesystem>
#include <stdexcept>
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
        throw std::runtime_error(
            "Failed to load editor icon '" + path.generic_string() + "': " +
            error.what()
        );
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
}

void EditorIcons::shutdown() {
    g_folder.reset();
    g_file.reset();
    g_texture.reset();
    g_model.reset();
    g_audio.reset();
    g_scene.reset();
    g_prefab.reset();
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
