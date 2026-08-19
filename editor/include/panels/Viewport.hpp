#pragma once

#include "EditorCamera.hpp"

#include <scene/Entity.hpp>

#include <memory>

namespace vshade::scene {
class Scene;
class SceneRenderer;
class SceneRuntime;
}

namespace vshade::renderer {
class Framebuffer;
}

namespace vshade::asset {
class AssetManager;
}

namespace editor {

class Viewport final {
public:
    enum class GizmoOperation {
        None,
        Translate,
        Rotate,
        Scale,
    };

    explicit Viewport(vshade::asset::AssetManager& assets);
    ~Viewport();

    Viewport(const Viewport&) = delete;
    Viewport& operator=(const Viewport&) = delete;

    void onUpdate(float deltaTime);
    void onImGuiRender(vshade::scene::Entity& selectedEntity);
    void setScene(std::shared_ptr<vshade::scene::Scene> scene);
    void setRuntime(vshade::scene::SceneRuntime* runtime) noexcept;
    void setEditing(bool editing) noexcept;
    void setVisible(bool visible) noexcept;
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void renderScene();
    void resizeFramebuffer(float width, float height);
    void drawGizmo(
        vshade::scene::Entity selectedEntity,
        float x,
        float y,
        float width,
        float height
    );
    void selectEntityUnderMouse(
        vshade::scene::Entity& selectedEntity,
        float x,
        float y,
        float width,
        float height
    );

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
    std::unique_ptr<vshade::scene::SceneRenderer> m_sceneRenderer;
    std::shared_ptr<vshade::scene::Scene> m_scene;
    vshade::scene::SceneRuntime* m_runtime = nullptr;
    EditorCamera m_editorCamera;
    GizmoOperation m_gizmoOperation = GizmoOperation::Translate;
    bool m_hovered = false;
    bool m_gizmoUsing = false;
    bool m_visible = true;
    bool m_editing = true;
    bool m_runtimeCameraActive = false;
};

} // namespace editor
