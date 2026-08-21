#pragma once

#include <asset/Asset.hpp>

#include <memory>

namespace vshade::renderer {
class Texture2D;
}

namespace editor {

/** @brief Loads and owns the editor's Content Browser type icons. */
class EditorIcons final {
public:
    EditorIcons() = delete;

    static void initialize();
    static void shutdown();

    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> folder();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> file();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> texture();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> model();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> audio();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> scene();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> prefab();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> camera();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> directionalLight();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> pointLight();
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> speaker();

    /** @brief Returns the icon for a file asset type. Folders use folder(). */
    [[nodiscard]] static std::shared_ptr<vshade::renderer::Texture2D> forAsset(
        vshade::asset::AssetType type
    );
};

} // namespace editor
