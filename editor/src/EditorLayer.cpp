#include "EditorLayer.hpp"

#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <renderer/Model.hpp>
#include <scene/Scene.hpp>

#include <algorithm>
#include <exception>
#include <memory>
#include <string>

#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace editor {
namespace {

constexpr const char* openSceneDialogKey = "OpenSceneDialog";
constexpr const char* importModelDialogKey = "ImportModelDialog";
constexpr const char* chooseProjectDialogKey = "ChooseProjectDialog";

} // namespace

EditorLayer::EditorLayer(vshade::asset::AssetManager& assets)
    : m_assets(&assets),
      m_viewport(assets) {
    m_inspectorPanel.setAssetManager(assets);
}

void EditorLayer::onAttach() {
    auto scene = std::make_shared<vshade::scene::Scene>("Editor Scene");
    SetEditorScene(scene);
}

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
    const vshade::scene::Entity selectedEntity =
        m_sceneHierarchyPanel.selectedEntity();
    m_viewport.setSelectedEntity(selectedEntity);
    m_viewport.onImGuiRender();
    m_inspectorPanel.onImGuiRender(selectedEntity);

    m_console.onImGuiRender();
    DrawFileDialogs();
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
        if (ImGui::MenuItem("New Scene")) {
            SetEditorScene(
                std::make_shared<vshade::scene::Scene>("Untitled Scene")
            );
        }
        if (ImGui::MenuItem("Open Scene...")) {
            IGFD::FileDialogConfig config;
            config.path = m_projectDirectory.empty()
                ? "."
                : m_projectDirectory.generic_string();
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog(
                openSceneDialogKey,
                "Open Scene",
                ".vscene,.json",
                config
            );
        }
        if (ImGui::MenuItem("Import Model...")) {
            IGFD::FileDialogConfig config;
            config.path = m_projectDirectory.empty()
                ? "."
                : m_projectDirectory.generic_string();
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog(
                importModelDialogKey,
                "Import Model",
                ".glb,.gltf",
                config
            );
        }
        if (ImGui::MenuItem("Choose Project Folder...")) {
            IGFD::FileDialogConfig config;
            config.path = m_projectDirectory.empty()
                ? "."
                : m_projectDirectory.generic_string();
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog(
                chooseProjectDialogKey,
                "Choose Project Folder",
                nullptr,
                config
            );
        }
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

void EditorLayer::DrawFileDialogs() {
    if (ImGuiFileDialog::Instance()->Display(openSceneDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk() && m_assets) {
            const std::string selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                const auto scene =
                    m_assets->loadResource<vshade::scene::Scene>(selectedPath);
                SetEditorScene(scene.shared());
            } catch (const std::exception& error) {
                ENGINE_ERROR(
                    "Failed to open scene '{}': {}",
                    selectedPath,
                    error.what()
                );
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(importModelDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk() && m_assets) {
            const std::string selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                static_cast<void>(
                    m_assets->load<vshade::renderer::Model>(selectedPath)
                );
            } catch (const std::exception& error) {
                ENGINE_ERROR(
                    "Failed to import model '{}': {}",
                    selectedPath,
                    error.what()
                );
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(chooseProjectDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_projectDirectory =
                ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void EditorLayer::SetEditorScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    if (!scene) {
        return;
    }
    m_editorScene = std::move(scene);
    m_sceneHierarchyPanel.setScene(m_editorScene);
    m_viewport.setScene(m_editorScene);
}

} // namespace editor
