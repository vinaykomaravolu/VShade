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
}

namespace vshade::asset {
class AssetManager;
}

namespace editor {

class EditorLayer final {
public:
    explicit EditorLayer(vshade::asset::AssetManager& assets);

    void onAttach();
    void onUpdate(float deltaTime);
    void onImGuiRender();
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void DrawDockspace();
    void DrawMenuBar();
    void DrawFileDialogs();
    void BuildDefaultDockLayout(std::uint32_t dockspaceId);
    void SetEditorScene(std::shared_ptr<vshade::scene::Scene> scene);

    vshade::asset::AssetManager* m_assets = nullptr;
    Console m_console;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    Viewport m_viewport;
    InspectorPanel m_inspectorPanel;
    std::shared_ptr<vshade::scene::Scene> m_editorScene;
    std::filesystem::path m_projectDirectory;
    bool m_resetDockLayoutRequested = false;
};

} // namespace editor
