#pragma once

#include "panels/Console.hpp"
#include "panels/ContentBrowserPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/Viewport.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>

namespace vshade::scene {
class Scene;
class SceneRuntime;
}

namespace vshade::asset {
class AssetManager;
}

namespace editor {

class Project;

class EditorLayer final {
public:
    enum class SceneState {
        Edit,
        Play,
        Pause,
    };

    EditorLayer(
        vshade::asset::AssetManager& assets,
        vshade::scene::SceneRuntime& runtime,
        std::function<void()> requestExit
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
    void loadScene(const std::filesystem::path& path);
    void saveScene();
    void saveSceneAs();
    void newProject();
    void openProject();
    void setProject(std::shared_ptr<Project> project);
    void importAsset(const std::filesystem::path& sourcePath);
    void duplicateSelectedEntity();
    void deleteSelectedEntity();
    void playScene();
    void pauseScene();
    void stepScene();
    void stopScene();
    void bindActiveScene();

    vshade::asset::AssetManager* m_assets = nullptr;
    vshade::scene::SceneRuntime* m_runtime = nullptr;
    std::function<void()> m_requestExit;
    Console m_console;
    ContentBrowserPanel m_contentBrowser;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    Viewport m_viewport;
    InspectorPanel m_inspectorPanel;
    vshade::scene::Entity m_selectedEntity;
    std::shared_ptr<vshade::scene::Scene> m_editorScene;
    std::shared_ptr<vshade::scene::Scene> m_runtimeScene;
    std::shared_ptr<Project> m_project;
    std::filesystem::path m_activeScenePath;
    SceneState m_sceneState = SceneState::Edit;
    bool m_stepRequested = false;
    bool m_resetDockLayoutRequested = false;
    bool m_showHierarchy = true;
    bool m_showInspector = true;
    bool m_showViewport = true;
    bool m_showConsole = true;
    bool m_showContentBrowser = true;
};

} // namespace editor
