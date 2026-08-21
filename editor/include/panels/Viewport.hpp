#pragma once

#include "EditorCamera.hpp"
#include "SceneEditHooks.hpp"

#include <math/Vector.hpp>
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

    enum class GizmoOrientation {
        Local,
        World,
    };

    enum class PivotMode {
        Object,
        Selection,
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
    void setEditHooks(SceneEditHooks hooks);
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    [[nodiscard]] bool drawOverlayToolbar(
        vshade::math::Vec2 position,
        vshade::math::Vec2 size
    );
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
    bool spawnDroppedAsset(
        vshade::scene::Entity& selectedEntity,
        float x,
        float y,
        float width,
        float height
    );
    void finishGizmoRecording();
    [[nodiscard]] vshade::math::Vec3 spawnPositionAtCursor(
        float x,
        float y,
        float width,
        float height
    ) const;

    std::unique_ptr<vshade::renderer::Framebuffer> m_framebuffer;
    std::unique_ptr<vshade::scene::SceneRenderer> m_sceneRenderer;
    vshade::asset::AssetManager* m_assets = nullptr;
    std::shared_ptr<vshade::scene::Scene> m_scene;
    vshade::scene::SceneRuntime* m_runtime = nullptr;
    EditorCamera m_editorCamera;
    SceneEditHooks m_editHooks;
    GizmoOperation m_gizmoOperation = GizmoOperation::Translate;
    GizmoOrientation m_gizmoOrientation = GizmoOrientation::Local;
    PivotMode m_pivotMode = PivotMode::Object;
    bool m_snapEnabled = false;
    float m_translationSnap = 0.5F;
    float m_rotationSnap = 15.0F;
    float m_scaleSnap = 0.1F;
    bool m_hovered = false;
    bool m_overlayHovered = false;
    bool m_gizmoUsing = false;
    bool m_gizmoRecording = false;
    bool m_visible = true;
    bool m_editing = true;
    bool m_runtimeCameraActive = false;
};

} // namespace editor
