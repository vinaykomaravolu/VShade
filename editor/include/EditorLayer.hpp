#pragma once

#include "panels/Console.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/Viewport.hpp"

#include <cstdint>
#include <memory>

namespace vshade::scene {
class Scene;
}

namespace editor {

class EditorLayer final {
public:
    void onAttach();
    void onUpdate(float deltaTime);
    void onImGuiRender();
    [[nodiscard]] bool wantsCursorCapture() const noexcept;

private:
    void DrawDockspace();
    void DrawMenuBar();
    void BuildDefaultDockLayout(std::uint32_t dockspaceId);

    Console m_console;
    SceneHierarchyPanel m_sceneHierarchyPanel;
    Viewport m_viewport;
    InspectorPanel m_inspectorPanel;
    std::shared_ptr<vshade::scene::Scene> m_editorScene;
    bool m_resetDockLayoutRequested = false;
};

} // namespace editor
