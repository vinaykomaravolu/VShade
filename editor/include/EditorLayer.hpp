#pragma once

#include "EditorDocument.hpp"
#include "UndoHistory.hpp"
#include "widgets/AssetSelector.hpp"
#include "panels/Console.hpp"
#include "panels/ContentBrowserPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/Viewport.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace vshade::scene {
class Scene;
class SceneRuntime;
}

namespace vshade::asset {
class AssetManager;
}

namespace vshade::script {
class NativeScriptRegistry;
}

namespace vshade::project {
class Project;
}

namespace editor {
class EditorLayer final {
public:
    enum class SceneState {
        Edit,
        Play,
        Pause,
    };

    enum class OperationTone {
        Neutral,
        Success,
        Warning,
        Error,
    };

    EditorLayer(
        vshade::asset::AssetManager& assets,
        vshade::scene::SceneRuntime& runtime,
        vshade::script::NativeScriptRegistry& scripts,
        std::function<void()> requestExit
    );

    ~EditorLayer();

    void onAttach();
    void onUpdate(float deltaTime);
    void onImGuiRender();
    void requestClose();
    [[nodiscard]] bool wantsCursorCapture() const noexcept;
    [[nodiscard]] std::string windowTitle() const;

private:
    void drawDockspace();
    void drawMenuBar();
    void drawToolbar();
    void drawStatusBar();
    void drawToasts();
    void drawFileDialogs();
    void drawUnsavedChangesModal();
    void buildDefaultDockLayout(std::uint32_t dockspaceId);
    void setActiveScene(
        std::shared_ptr<vshade::scene::Scene> scene,
        std::filesystem::path path = {}
    );
    void newScene();
    void openScene();
    bool loadScene(const std::filesystem::path& path);
    bool saveScene();
    bool saveSceneTo(const std::filesystem::path& path);
    void saveSceneAs();
    void newProject();
    void openProject();
    bool setProject(std::shared_ptr<vshade::project::Project> project);
    void loadProjectCatalog();
    void saveProjectCatalog();
    void recordStartSceneIfUnset();
    [[nodiscard]] std::optional<std::filesystem::path> importAsset(
        const std::filesystem::path& sourcePath
    );
    void saveSelectedAsPrefab();
    void writePrefab(const std::filesystem::path& path);
    void duplicateSelectedEntity();
    void deleteSelectedEntity();
    void beginSceneEdit();
    void commitSceneEdit();
    void revertSceneEdit();
    void cancelSceneEdit();
    void undoSceneEdit();
    void redoSceneEdit();
    void requestTransition(std::function<void()> action);
    void completePendingTransition();
    void cancelPendingTransition();
    void requestExit();
    void handleEditHotkeys();
    void playScene();
    void pauseScene();
    void stepScene();
    void stopScene();
    void bindActiveScene();
    void setOperationResult(std::string message, OperationTone tone);

    vshade::asset::AssetManager* m_assets = nullptr;
    vshade::scene::SceneRuntime* m_runtime = nullptr;
    vshade::script::NativeScriptRegistry* m_scripts = nullptr;
    std::function<void()> m_requestExit;
    Console m_console;
    ContentBrowserPanel m_contentBrowser;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    Viewport m_viewport;
    InspectorPanel m_inspectorPanel;
    AssetSelector m_assetSelector;
    UndoHistory m_undoHistory;
    EditorDocument m_document;
    vshade::scene::Entity m_selectedEntity;
    std::shared_ptr<vshade::scene::Scene> m_editorScene;
    std::shared_ptr<vshade::scene::Scene> m_runtimeScene;
    std::shared_ptr<vshade::project::Project> m_project;
    std::function<void()> m_pendingTransition;
    SceneState m_sceneState = SceneState::Edit;
    bool m_stepRequested = false;
    bool m_resetDockLayoutRequested = false;
    bool m_openUnsavedChangesRequested = false;
    bool m_continueAfterSave = false;
    bool m_showHierarchy = true;
    bool m_showInspector = true;
    bool m_showViewport = true;
    bool m_showConsole = true;
    bool m_showContentBrowser = true;
    std::string m_importActivity = "Idle";
    std::string m_buildActivity = "Idle";
    std::string m_lastOperation = "Ready";
    OperationTone m_lastOperationTone = OperationTone::Neutral;
    float m_toastSecondsRemaining = 0.0F;
};

} // namespace editor
