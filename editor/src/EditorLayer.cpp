#include "EditorLayer.hpp"

#include <algorithm>

#include <imgui.h>
#include <imgui_internal.h>

namespace editor {

void EditorLayer::onAttach() {}

void EditorLayer::onUpdate(const float deltaTime) {
    m_viewport.onUpdate(deltaTime);
}

void EditorLayer::onImGuiRender() {
    DrawDockspace();
}

bool EditorLayer::wantsCursorCapture() const noexcept {
    return m_viewport.wantsCursorCapture();
}

void EditorLayer::DrawDockspace()
{
    DrawMenuBar();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    const ImVec2 hostPosition = viewport->WorkPos;
    const ImVec2 hostSize = viewport->WorkSize;

    constexpr ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoNavFocus
        | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(hostPosition);
    ImGui::SetNextWindowSize(hostSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2{0.0F, 0.0F}
    );

    ImGui::Begin(
        "VShadeDockspaceHost",
        nullptr,
        hostFlags
    );

    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId =
        ImGui::GetID("VShadeDockspace");

    if (m_resetDockLayoutRequested) {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        m_resetDockLayoutRequested = false;
    }

    if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
        BuildDefaultDockLayout(dockspaceId);
    }

    ImGui::DockSpace(
        dockspaceId,
        ImVec2{0.0F, 0.0F}
    );

    ImGui::End();

    m_sceneHierarchyPanel.onImGuiRender();
    m_viewport.onImGuiRender();

    m_inspectorPanel.onImGuiRender(
        m_sceneHierarchyPanel.selectedEntity()
    );

    m_console.onImGuiRender();
}

void EditorLayer::BuildDefaultDockLayout(const std::uint32_t dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodePos(dockspaceId, ImGui::GetWindowPos());
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetWindowSize());

    ImGuiID centerDockId = dockspaceId;
    const ImGuiID consoleDockId = ImGui::DockBuilderSplitNode(
        centerDockId,
        ImGuiDir_Down,
        0.25F,
        nullptr,
        &centerDockId
    );
    const ImGuiID hierarchyDockId = ImGui::DockBuilderSplitNode(
        centerDockId,
        ImGuiDir_Left,
        0.20F,
        nullptr,
        &centerDockId
    );
    const ImGuiID inspectorDockId = ImGui::DockBuilderSplitNode(
        centerDockId,
        ImGuiDir_Right,
        0.25F,
        nullptr,
        &centerDockId
    );

    ImGui::DockBuilderDockWindow("Hierarchy", hierarchyDockId);
    ImGui::DockBuilderDockWindow("Viewport", centerDockId);
    ImGui::DockBuilderDockWindow("Inspector", inspectorDockId);
    ImGui::DockBuilderDockWindow("Console", consoleDockId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("New Scene");
        ImGui::MenuItem("Open Scene...");
        ImGui::Separator();
        ImGui::MenuItem("Exit");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Undo");
        ImGui::MenuItem("Redo");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Reset Layout")) {
            m_resetDockLayoutRequested = true;
        }
        ImGui::EndMenu();
    }

    const float titleWidth = ImGui::CalcTextSize("VShade").x;
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - titleWidth - 12.0F)
    );
    ImGui::TextUnformatted("VShade");
    ImGui::EndMainMenuBar();
}

} // namespace editor
