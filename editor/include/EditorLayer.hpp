#pragma once

#include "panels/Console.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneHierarchyPanel.hpp"
#include "panels/Viewport.hpp"

#include <cstdint>

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
    bool m_resetDockLayoutRequested = false;
};

} // namespace editor
