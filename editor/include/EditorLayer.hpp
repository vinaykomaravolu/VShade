#pragma once

#include "panels/Console.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/Viewport.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace vshade::scene {
class Scene;
class SceneRuntime;
}

namespace vshade::asset {
class AssetManager;
}

namespace editor {

class EditorLayer final {
public:
    enum class SceneState {
        Edit,
        Play,
        Pause,
    };

    EditorLayer(
        vshade::asset::AssetManager& assets,
        vshade::scene::SceneRuntime& runtime
    );

    void onAttach();
    void onUpdate(float deltaTime);
    void onImGuiRender();
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void DrawDockspace();
    void DrawMenuBar();
    void DrawToolbar();
    void DrawFileDialogs();
    void BuildDefaultDockLayout(std::uint32_t dockspaceId);
    void setActiveScene(std::shared_ptr<vshade::scene::Scene> scene);
    void newScene();
    void openScene();
    void saveScene();
    void saveSceneAs();
    void playScene();
    void stopScene();
    void bindActiveScene();

    vshade::asset::AssetManager* m_assets = nullptr;
    vshade::scene::SceneRuntime* m_runtime = nullptr;
    Console m_console;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    Viewport m_viewport;
    InspectorPanel m_inspectorPanel;
    std::shared_ptr<vshade::scene::Scene> m_editorScene;
    std::shared_ptr<vshade::scene::Scene> m_runtimeScene;
    std::filesystem::path m_activeScenePath;
    std::filesystem::path m_projectDirectory;
    SceneState m_sceneState = SceneState::Edit;
    bool m_resetDockLayoutRequested = false;
};

} // namespace editor
