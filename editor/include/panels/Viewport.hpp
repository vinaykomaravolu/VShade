#pragma once

#include "EditorCamera.hpp"

#include <scene/Entity.hpp>

#include <memory>

namespace vshade::scene {
class Scene;
}

namespace vshade::renderer {
class Framebuffer;
}

namespace editor {

class Viewport final {
public:
    Viewport();
    ~Viewport();

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void onUpdate(float deltaTime);
    void onImGuiRender();
    void setScene(
        std::shared_ptr<vshade::scene::Scene> scene,
        vshade::scene::Entity previewEntity
    );
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void renderScene();
    void resizeFramebuffer(float width, float height);

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
    std::shared_ptr<vshade::scene::Scene> m_scene;
    vshade::scene::Entity m_previewEntity;
    EditorCamera m_editorCamera;
    bool m_hovered = false;
};

} // namespace editor
