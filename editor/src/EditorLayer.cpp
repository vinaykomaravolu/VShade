#include "EditorLayer.hpp"
#include "AssetImportService.hpp"
#include "SceneEditHooks.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <core/Log.hpp>
#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <project/Project.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Texture.hpp>
#include <scene/Prefab.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneRuntime.hpp>
#include <scene/SceneSerializer.hpp>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#endif

#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "ImGui/ImGuiTheme.hpp"

namespace editor {
namespace {

constexpr const char* openSceneDialogKey = "OpenSceneDialog";
constexpr const char* saveSceneDialogKey = "SaveSceneDialog";
constexpr const char* importAssetDialogKey = "ImportAssetDialog";
constexpr const char* openProjectDialogKey = "OpenProjectDialog";
constexpr const char* newProjectDialogKey = "NewProjectDialog";
constexpr const char* savePrefabDialogKey = "SavePrefabDialog";
constexpr const char* unsavedChangesPopup = "Unsaved Scene Changes";

[[nodiscard]] bool replaceFileAtomically(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& errorMessage
) {
#if defined(_WIN32)
    if (MoveFileExW(
            source.c_str(),
            destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        ) != 0) {
        return true;
    }
    const std::error_code error(
        static_cast<int>(GetLastError()),
        std::system_category()
    );
#else
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    if (!error) {
        return true;
    }
#endif
    errorMessage = error.message();
    return false;
}

[[nodiscard]] bool serializeSceneAtomically(
    const vshade::scene::Scene& scene,
    const std::filesystem::path& path,
    std::string& errorMessage
) {
    if (path.empty()) {
        errorMessage = "A scene path is required";
        return false;
    }

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::error_code filesystemError;
    std::filesystem::remove(temporary, filesystemError);

    vshade::scene::SceneSerializer serializer(scene);
    if (!serializer.serialize(
            temporary,
            vshade::scene::SceneJsonFormat::Compact
        )) {
        errorMessage = serializer.lastError();
        return false;
    }
    if (replaceFileAtomically(temporary, path, errorMessage)) {
        return true;
    }
    filesystemError.clear();
    std::filesystem::remove(temporary, filesystemError);
    return false;
}

void loadProjectCatalogInto(
    vshade::asset::AssetManager& assets,
    const vshade::project::Project& project
) {
    assets.setRootDirectory(project.projectDirectory());
    assets.clearCatalog();
    const std::filesystem::path catalogPath = project.assetRegistryPath();
    std::error_code error;
    if (std::filesystem::is_regular_file(catalogPath, error) && !error) {
        assets.loadCatalog(catalogPath);
    }
}

} // namespace

EditorLayer::EditorLayer(
    vshade::asset::AssetManager& assets,
    vshade::scene::SceneRuntime& runtime,
    vshade::script::NativeScriptRegistry& scripts,
    std::function<void()> requestExit
)
    : m_assets(&assets),
      m_runtime(&runtime),
      m_scripts(&scripts),
      m_requestExit(std::move(requestExit)),
      m_contentBrowser(std::filesystem::path{}),
      m_viewport(assets) {
    m_contentBrowser.setAssetManager(assets);
    m_contentBrowser.setOperationHandler(
        [this](std::string message, const bool success) {
            setOperationResult(
                std::move(message),
                success ? OperationTone::Success : OperationTone::Error
            );
        }
    );
    m_inspectorPanel.setAssetManager(assets);
    m_inspectorPanel.setAssetSelector(m_assetSelector);
    m_inspectorPanel.setScriptRegistry(scripts);
    m_inspectorPanel.setRevealAssetHandler(
        [this](std::filesystem::path path) {
            m_showContentBrowser = true;
            m_contentBrowser.reveal(path);
        }
    );
    m_assetSelector.setCatalogChangedCallback([this] {
        saveProjectCatalog();
    });
    m_assetSelector.setImportCallback(
        [this](const std::filesystem::path& source) {
            return importAsset(source);
        }
    );
    m_sceneHierarchyPanel.setSavePrefabHandler([this](vshade::scene::Entity entity) {
        m_selectedEntity = entity;
        saveSelectedAsPrefab();
    });
    const SceneEditHooks editHooks{
        .begin = [this] { beginSceneEdit(); },
        .commit = [this] { commitSceneEdit(); },
        .revert = [this] { revertSceneEdit(); },
        .cancel = [this] { cancelSceneEdit(); },
    };
    m_sceneHierarchyPanel.setEditHooks(editHooks);
    m_viewport.setEditHooks(editHooks);
    m_inspectorPanel.setEditHooks(editHooks);
}

EditorLayer::~EditorLayer() {
    m_assetSelector.setCatalogChangedCallback({});
    m_assetSelector.setImportCallback({});
}

void EditorLayer::onAttach() {
    newScene();
}

void EditorLayer::onUpdate(const float deltaTime) {
    m_toastSecondsRemaining = std::max(
        0.0F,
        m_toastSecondsRemaining - deltaTime
    );
    if (m_sceneState == SceneState::Pause
        && m_stepRequested
        && m_runtime
        && m_runtime->isPlaying()) {
        constexpr float fixedStep = 1.0F / 60.0F;
        m_runtime->fixedUpdate(fixedStep);
        m_runtime->update(fixedStep);
        m_stepRequested = false;
    }
    m_viewport.onUpdate(deltaTime);
}

void EditorLayer::onImGuiRender() {
    drawDockspace();
}

void EditorLayer::requestClose() {
    requestExit();
}

bool EditorLayer::wantsCursorCapture() const noexcept {
    return m_viewport.wantsCursorCapture();
}

std::string EditorLayer::windowTitle() const {
    std::string title = m_document.displayName();
    if (m_document.isDirty()) {
        title += " *";
    }
    title += " - VShade Editor";
    return title;
}

void EditorLayer::drawDockspace()
{
    m_viewport.setVisible(m_showViewport);
    m_viewport.setEditing(m_sceneState == SceneState::Edit);
    m_viewport.setRuntime(m_runtime);
    drawMenuBar();
    drawToolbar();
    drawStatusBar();

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
        buildDefaultDockLayout(dockspaceId);
    }

    ImGui::DockSpace(
        dockspaceId,
        ImVec2{0.0F, 0.0F}
    );

    ImGui::End();

    if (m_showHierarchy) {
        m_sceneHierarchyPanel.setReadOnly(m_sceneState != SceneState::Edit);
        m_sceneHierarchyPanel.onImGuiRender(m_selectedEntity);
    }
    if (m_showViewport) {
        m_viewport.onImGuiRender(m_selectedEntity);
    }
    if (m_showInspector) {
        m_inspectorPanel.setReadOnly(m_sceneState != SceneState::Edit);
        m_inspectorPanel.onImGuiRender(m_selectedEntity);
    }

    if (m_showContentBrowser) {
        if (const auto scenePath = m_contentBrowser.onImGuiRender();
            scenePath && m_sceneState == SceneState::Edit) {
            requestTransition([this, path = *scenePath] {
                static_cast<void>(loadScene(path));
            });
        }
    }
    if (m_showConsole) {
        m_console.onImGuiRender();
    }
    drawFileDialogs();
    drawUnsavedChangesModal();
    drawToasts();
}

void EditorLayer::drawToasts() {
    if (m_toastSecondsRemaining <= 0.0F || m_lastOperation.empty()) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 padding = ui::scaled(18.0F, 42.0F);
    ImGui::SetNextWindowPos(
        {
            viewport->WorkPos.x + viewport->WorkSize.x - padding.x,
            viewport->WorkPos.y + viewport->WorkSize.y - padding.y,
        },
        ImGuiCond_Always,
        {1.0F, 1.0F}
    );
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.94F);
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ui::scaled(8.0F));
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ui::scaled(14.0F, 10.0F)
    );
    ImGui::Begin("##OperationToast", nullptr, flags);
    ImVec4 tone = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    switch (m_lastOperationTone) {
        case OperationTone::Success:
            tone = ui::color(ui::ColorRole::Success);
            break;
        case OperationTone::Warning:
            tone = ui::color(ui::ColorRole::Warning);
            break;
        case OperationTone::Error:
            tone = ui::color(ui::ColorRole::Error);
            break;
        case OperationTone::Neutral:
            tone = ui::color(ui::ColorRole::Accent);
            break;
    }
    ImGui::TextColored(tone, "●");
    ImGui::SameLine();
    ImGui::TextUnformatted(m_lastOperation.c_str());
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void EditorLayer::buildDefaultDockLayout(const std::uint32_t dockspaceId) {
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
    ImGui::DockBuilderDockWindow("Content Browser", consoleDockId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::drawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            requestTransition([this] { newScene(); });
        }
        if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
            requestTransition([this] { openScene(); });
        }
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            saveScene();
        }
        if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
            saveSceneAs();
        }
        ImGui::BeginDisabled(
            !m_editorScene || !m_editorScene->valid(m_selectedEntity)
        );
        if (ImGui::MenuItem("Save as Prefab...")) {
            saveSelectedAsPrefab();
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::BeginDisabled(!m_project);
        if (ImGui::MenuItem("Import Asset...")) {
            IGFD::FileDialogConfig config;
            config.path = m_project->assetDirectory().generic_string();
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            ImGuiFileDialog::Instance()->OpenDialog(
                importAssetDialogKey,
                "Import Asset",
                ".glb,.gltf,.png,.jpg,.jpeg,.bmp,.tga,.wav,.mp3,.flac,.ogg,.vsprefab",
                config
            );
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
        if (ImGui::MenuItem("Open Project...")) {
            requestTransition([this] { openProject(); });
        }
        if (ImGui::MenuItem("New Project...")) {
            requestTransition([this] { newProject(); });
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) {
            requestExit();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        const bool editing = m_sceneState == SceneState::Edit;
        ImGui::BeginDisabled(!editing || !m_undoHistory.canUndo());
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            undoSceneEdit();
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!editing || !m_undoHistory.canRedo());
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
            redoSceneEdit();
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        const std::shared_ptr<vshade::scene::Scene>& activeScene =
            m_sceneState == SceneState::Edit
                ? m_editorScene
                : m_runtimeScene;
        const bool hasSelection =
            editing
            && activeScene
            && activeScene->valid(m_selectedEntity);
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D")) {
            duplicateSelectedEntity();
        }
        if (ImGui::MenuItem("Save as Prefab...")) {
            saveSelectedAsPrefab();
        }
        ui::pushDestructiveTextStyle();
        if (ImGui::MenuItem("Delete Entity", "Delete")) {
            deleteSelectedEntity();
        }
        ui::popDestructiveTextStyle();
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
    handleEditHotkeys();
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
        ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
        ImGui::MenuItem("Console", nullptr, &m_showConsole);
        ImGui::MenuItem(
            "Content Browser",
            nullptr,
            &m_showContentBrowser
        );
        ImGui::Separator();
        if (ImGui::MenuItem("Reset Layout")) {
            m_resetDockLayoutRequested = true;
        }
        ImGui::EndMenu();
    }

    const std::string projectName = m_project
        ? m_project->config().name
        : "No Project";
    std::string chrome = projectName + "  /  " + m_document.displayName();
    if (m_document.isDirty()) {
        chrome += "  [Modified]";
    }
    const char* state = "EDIT";
    ui::ColorRole stateColor = ui::ColorRole::Muted;
    if (m_sceneState == SceneState::Play) {
        state = "PLAY";
        stateColor = ui::ColorRole::Success;
    } else if (m_sceneState == SceneState::Pause) {
        state = "PAUSED";
        stateColor = ui::ColorRole::Warning;
    }
    const float chromeWidth = ImGui::CalcTextSize(chrome.c_str()).x
        + ImGui::CalcTextSize(state).x
        + ui::scaled(ui::Metrics::chromePadding * 2.0F);
    ImGui::SetCursorPosX(
        std::max(
            ImGui::GetCursorPosX(),
            ImGui::GetWindowWidth() - chromeWidth
        )
    );
    ImGui::TextUnformatted(chrome.c_str());
    ImGui::SameLine(0.0F, ui::scaled(ui::Metrics::chromePadding));
    ImGui::TextColored(ui::color(stateColor), "%s", state);
    ImGui::EndMainMenuBar();
}

void EditorLayer::drawToolbar() {
    constexpr ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::BeginViewportSideBar(
            "##VShadeToolbar",
            ImGui::GetMainViewport(),
            ImGuiDir_Up,
            ui::scaled(ui::Metrics::toolbarHeight),
            toolbarFlags
        )) {
        return;
    }

    const float buttonWidth = ui::scaled(ui::Metrics::toolbarButtonWidth);
    const float spacing = ui::scaled(ui::Metrics::spacing);
    const float controlHeight = ui::scaled(ui::Metrics::controlHeight);
    const float totalWidth = buttonWidth * 4.0F + spacing * 3.0F;
    const float contentY = ui::scaled(5.0F);

    ImGui::SetCursorPosX(
        std::max(0.0F, (ImGui::GetWindowWidth() - totalWidth) * 0.5F)
    );
    ImGui::SetCursorPosY(contentY);

    ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
    if (ui::iconButton(
            "##Play",
            ui::Icon::Play,
            {buttonWidth, controlHeight},
            "Play"
        )) {
        playScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState == SceneState::Edit);
    const bool paused = m_sceneState == SceneState::Pause;
    if (ui::iconButton(
            "##Pause",
            paused ? ui::Icon::Play : ui::Icon::Pause,
            {buttonWidth, controlHeight},
            paused ? "Resume" : "Pause"
        )) {
        pauseScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState != SceneState::Pause);
    if (ui::iconButton(
            "##Step",
            ui::Icon::Step,
            {buttonWidth, controlHeight},
            "Step one frame"
        )) {
        stepScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState == SceneState::Edit);
    if (ui::iconButton(
            "##Stop",
            ui::Icon::Stop,
            {buttonWidth, controlHeight},
            "Stop"
        )) {
        stopScene();
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void EditorLayer::drawStatusBar() {
    constexpr ImGuiWindowFlags statusFlags =
        ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoNav;
    if (!ImGui::BeginViewportSideBar(
            "##VShadeStatusBar",
            ImGui::GetMainViewport(),
            ImGuiDir_Down,
            ui::scaled(ui::Metrics::statusBarHeight),
            statusFlags
        )) {
        return;
    }

    const auto separator = [] {
        ImGui::SameLine(0.0F, ui::scaled(ui::Metrics::spacing));
        ImGui::TextDisabled("|");
        ImGui::SameLine(0.0F, ui::scaled(ui::Metrics::spacing));
    };

    const std::shared_ptr<vshade::scene::Scene>& activeScene =
        m_sceneState == SceneState::Edit ? m_editorScene : m_runtimeScene;
    const bool hasSelection = activeScene && activeScene->valid(m_selectedEntity);
    const std::string selection = hasSelection
        ? std::string(m_selectedEntity.name())
        : "None";
    ImGui::Text("Selection: %s", selection.c_str());
    separator();
    ImGui::Text("Import: %s", m_importActivity.c_str());
    separator();
    ImGui::Text("Build: %s", m_buildActivity.c_str());
    separator();

    ui::ColorRole resultColor = ui::ColorRole::Muted;
    switch (m_lastOperationTone) {
        case OperationTone::Success: resultColor = ui::ColorRole::Success; break;
        case OperationTone::Warning: resultColor = ui::ColorRole::Warning; break;
        case OperationTone::Error: resultColor = ui::ColorRole::Error; break;
        case OperationTone::Neutral: break;
    }
    ImGui::TextColored(
        ui::color(resultColor),
        "Last: %s",
        m_lastOperation.c_str()
    );

    const ImGuiIO& io = ImGui::GetIO();
    const float fps = io.Framerate;
    const float milliseconds = fps > 0.0F ? 1000.0F / fps : 0.0F;
    const vshade::renderer::RenderStats& stats =
        vshade::renderer::Renderer::stats();
    const std::string frameStats = std::format(
        "{:.0f} FPS  {:.2f} ms  Draws: {}  Tris: {}",
        fps,
        milliseconds,
        stats.drawCalls,
        stats.triangleCount
    );
    const float statsWidth = ImGui::CalcTextSize(frameStats.c_str()).x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(std::max(
        ImGui::GetCursorPosX(),
        ImGui::GetWindowWidth()
            - statsWidth
            - ui::scaled(ui::Metrics::chromePadding)
    ));
    ImGui::TextUnformatted(frameStats.c_str());
    ImGui::End();
}

void EditorLayer::drawUnsavedChangesModal() {
    if (m_openUnsavedChangesRequested) {
        ImGui::OpenPopup(unsavedChangesPopup);
        m_openUnsavedChangesRequested = false;
    }

    if (!ImGui::BeginPopupModal(
            unsavedChangesPopup,
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
        return;
    }

    ImGui::Text(
        "Save changes to %s before continuing?",
        m_document.displayName().c_str()
    );
    ImGui::TextDisabled("Unsaved scene changes will be lost if discarded.");
    ImGui::Separator();

    const ImVec2 modalButtonSize = ui::scaled(100.0F, 0.0F);
    if (ImGui::Button("Save", modalButtonSize)) {
        if (m_document.path().empty()) {
            m_continueAfterSave = true;
            saveSceneAs();
            ImGui::CloseCurrentPopup();
        } else if (saveScene()) {
            ImGui::CloseCurrentPopup();
            completePendingTransition();
        }
    }
    ImGui::SameLine();
    ui::pushDestructiveButtonStyle();
    if (ImGui::Button("Discard", modalButtonSize)) {
        ImGui::CloseCurrentPopup();
        completePendingTransition();
    }
    ui::popDestructiveButtonStyle();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", modalButtonSize)) {
        ImGui::CloseCurrentPopup();
        cancelPendingTransition();
    }

    ImGui::EndPopup();
}

void EditorLayer::drawFileDialogs() {
    if (ImGuiFileDialog::Instance()->Display(openSceneDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            loadScene(selectedPath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(saveSceneDialogKey)) {
        bool saved = false;
        const bool accepted = ImGuiFileDialog::Instance()->IsOk();
        if (accepted) {
            saved = saveSceneTo(
                ImGuiFileDialog::Instance()->GetFilePathName(
                    IGFD_ResultMode_OverwriteFileExt
                )
            );
        }
        ImGuiFileDialog::Instance()->Close();
        if (m_continueAfterSave) {
            m_continueAfterSave = false;
            if (saved) {
                completePendingTransition();
            } else if (accepted) {
                m_openUnsavedChangesRequested = true;
            } else {
                cancelPendingTransition();
            }
        }
    }

    if (ImGuiFileDialog::Instance()->Display(importAssetDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            static_cast<void>(importAsset(selectedPath));
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(openProjectDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                static_cast<void>(setProject(
                    vshade::project::Project::load(selectedPath)
                ));
            } catch (const std::exception& error) {
                setOperationResult("Project open failed", OperationTone::Error);
                ENGINE_ERROR(
                    "Failed to open project '{}': {}",
                    selectedPath.generic_string(),
                    error.what()
                );
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(newProjectDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedDirectory =
                ImGuiFileDialog::Instance()->GetCurrentPath();
            try {
                static_cast<void>(setProject(
                    vshade::project::Project::create(selectedDirectory)
                ));
            } catch (const std::exception& error) {
                setOperationResult("Project creation failed", OperationTone::Error);
                ENGINE_ERROR(
                    "Failed to create project '{}': {}",
                    selectedDirectory.generic_string(),
                    error.what()
                );
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(savePrefabDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            writePrefab(ImGuiFileDialog::Instance()->GetFilePathName(
                IGFD_ResultMode_OverwriteFileExt
            ));
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void EditorLayer::setActiveScene(
    std::shared_ptr<vshade::scene::Scene> scene,
    std::filesystem::path path
) {
    if (!scene) {
        return;
    }
    m_editorScene = std::move(scene);
    m_undoHistory.clear();
    m_document.reset(std::move(path), m_undoHistory.currentRevision());
    bindActiveScene();
}

void EditorLayer::newScene() {
    setActiveScene(
        std::make_shared<vshade::scene::Scene>("Untitled Scene")
    );
    setOperationResult("Created a new scene", OperationTone::Success);
}

void EditorLayer::openScene() {
    IGFD::FileDialogConfig config;
    config.path = m_project
        ? m_project->sceneDirectory().generic_string()
        : ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        openSceneDialogKey,
        "Open Scene",
        ".vscene,.json",
        config
    );
}

bool EditorLayer::loadScene(const std::filesystem::path& path) {
    if (m_sceneState != SceneState::Edit) {
        return false;
    }

    auto scene = std::make_shared<vshade::scene::Scene>(
        path.stem().string()
    );
    vshade::scene::SceneSerializer serializer(*scene);
    if (!serializer.deserialize(path)) {
        setOperationResult("Scene open failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to open scene '{}': {}",
            path.generic_string(),
            serializer.lastError()
        );
        return false;
    }

    try {
        if (m_assets) {
            scene->applyPrefabInstances(*m_assets);
        }
    } catch (const std::exception& error) {
        setOperationResult("Scene asset resolution failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to resolve scene assets '{}': {}",
            path.generic_string(),
            error.what()
        );
        return false;
    }
    setActiveScene(std::move(scene), path);
    setOperationResult(
        "Opened " + path.filename().generic_string(),
        OperationTone::Success
    );
    return true;
}

bool EditorLayer::saveScene() {
    if (!m_editorScene) {
        return false;
    }
    if (m_document.path().empty()) {
        saveSceneAs();
        return false;
    }
    return saveSceneTo(m_document.path());
}

bool EditorLayer::saveSceneTo(const std::filesystem::path& path) {
    if (!m_editorScene || path.empty()) {
        return false;
    }

    std::string error;
    if (!serializeSceneAtomically(*m_editorScene, path, error)) {
        setOperationResult("Scene save failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to save scene '{}': {}",
            path.generic_string(),
            error
        );
        return false;
    }
    m_document.setPath(path);
    m_document.markSaved();
    ENGINE_INFO("Saved scene '{}'", path.generic_string());
    setOperationResult(
        "Saved " + path.filename().generic_string(),
        OperationTone::Success
    );
    recordStartSceneIfUnset();
    return true;
}

void EditorLayer::saveSceneAs() {
    if (!m_editorScene) {
        return;
    }

    IGFD::FileDialogConfig config;
    if (!m_document.path().empty()) {
        config.path = m_document.path().parent_path().generic_string();
        config.fileName = m_document.path().filename().generic_string();
    } else {
        config.path = m_project
            ? m_project->sceneDirectory().generic_string()
            : ".";
        config.fileName = "Untitled.vscene";
    }
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal
        | ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog(
        saveSceneDialogKey,
        "Save Scene As",
        ".vscene",
        config
    );
}

void EditorLayer::newProject() {
    IGFD::FileDialogConfig config;
    config.path = m_project
        ? m_project->projectDirectory().generic_string()
        : ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        newProjectDialogKey,
        "Create Project Directory",
        nullptr,
        config
    );
}

void EditorLayer::openProject() {
    IGFD::FileDialogConfig config;
    config.path = m_project
        ? m_project->projectDirectory().generic_string()
        : ".";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        openProjectDialogKey,
        "Open Project",
        ".vshade",
        config
    );
}

bool EditorLayer::setProject(
    std::shared_ptr<vshade::project::Project> project
) {
    if (!project || !m_assets || m_sceneState != SceneState::Edit) {
        return false;
    }

    std::shared_ptr<vshade::scene::Scene> nextScene;
    const std::filesystem::path startScene = project->startScenePath();
    try {
        vshade::asset::AssetManager candidateAssets;
        loadProjectCatalogInto(candidateAssets, *project);

        if (startScene.empty()) {
            nextScene = std::make_shared<vshade::scene::Scene>("Untitled Scene");
        } else {
            if (!std::filesystem::is_regular_file(startScene)) {
                throw std::runtime_error(
                    "Project start scene does not exist: "
                    + startScene.generic_string()
                );
            }
            nextScene = std::make_shared<vshade::scene::Scene>(
                startScene.stem().string()
            );
            vshade::scene::SceneSerializer serializer(*nextScene);
            if (!serializer.deserialize(startScene)) {
                throw std::runtime_error(serializer.lastError());
            }
            nextScene->applyPrefabInstances(candidateAssets);
        }
    } catch (const std::exception& error) {
        setOperationResult("Project preparation failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to prepare project '{}': {}",
            project->projectFile().generic_string(),
            error.what()
        );
        return false;
    }

    const std::shared_ptr<vshade::project::Project> previousProject = m_project;
    try {
        loadProjectCatalogInto(*m_assets, *project);
    } catch (const std::exception& error) {
        setOperationResult("Project activation failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to activate project '{}': {}",
            project->projectFile().generic_string(),
            error.what()
        );
        try {
            if (previousProject) {
                loadProjectCatalogInto(*m_assets, *previousProject);
            } else {
                m_assets->clearCatalog();
                m_assets->setRootDirectory({});
            }
        } catch (const std::exception& rollbackError) {
            ENGINE_CRITICAL(
                "Failed to restore the previous asset catalog: {}",
                rollbackError.what()
            );
        }
        return false;
    }

    m_project = std::move(project);
    m_contentBrowser.setRoot(m_project->assetDirectory());
    m_assetSelector.setSearchDirectory(m_project->assetDirectory());
    m_assetSelector.discover(*m_assets);
    setActiveScene(std::move(nextScene), startScene);
    m_continueAfterSave = false;
    cancelPendingTransition();
    ENGINE_INFO(
        "Opened project '{}'",
        m_project->projectFile().generic_string()
    );
    setOperationResult(
        "Opened project " + m_project->config().name,
        OperationTone::Success
    );
    return true;
}

void EditorLayer::loadProjectCatalog() {
    if (!m_project || !m_assets) {
        return;
    }

    try {
        loadProjectCatalogInto(*m_assets, *m_project);
    } catch (const std::exception& catalogError) {
        ENGINE_ERROR(
            "Failed to load asset catalog '{}': {}",
            m_project->assetRegistryPath().generic_string(),
            catalogError.what()
        );
    }
}

void EditorLayer::saveProjectCatalog() {
    if (!m_project || !m_assets) {
        return;
    }

    try {
        m_assets->saveCatalog(m_project->assetRegistryPath());
    } catch (const std::exception& catalogError) {
        ENGINE_ERROR(
            "Failed to save asset catalog '{}': {}",
            m_project->assetRegistryPath().generic_string(),
            catalogError.what()
        );
    }
}

std::optional<std::filesystem::path> EditorLayer::importAsset(
    const std::filesystem::path& sourcePath
) {
    if (!m_project || !m_assets || sourcePath.empty()) {
        return std::nullopt;
    }

    m_importActivity = "Importing " + sourcePath.filename().generic_string();
    try {
        const vshade::asset::AssetType type =
            vshade::asset::assetTypeFromExtension(sourcePath.extension());
        const AssetImportResult imported = AssetImportService::import(
            *m_project,
            sourcePath,
            AssetCollisionPolicy::KeepBoth
        );
        if (!imported) {
            if (imported.cancelled) {
                m_importActivity = "Idle";
                setOperationResult("Import cancelled", OperationTone::Warning);
                return std::nullopt;
            }
            throw std::runtime_error(imported.error);
        }
        const std::filesystem::path& destination = imported.destination;

        switch (type) {
            case vshade::asset::AssetType::Model:
                static_cast<void>(
                    m_assets->load<vshade::renderer::Model>(destination)
                );
                break;
            case vshade::asset::AssetType::Texture:
                static_cast<void>(
                    m_assets->load<vshade::renderer::Texture2D>(destination)
                );
                break;
            case vshade::asset::AssetType::Audio:
                static_cast<void>(
                    m_assets->load<vshade::audio::AudioClip>(destination)
                );
                break;
            case vshade::asset::AssetType::Prefab:
                static_cast<void>(
                    m_assets->load<vshade::scene::Prefab>(destination)
                );
                break;
            default:
                throw std::invalid_argument("Unsupported asset file type");
        }
        ENGINE_INFO("Imported asset '{}'", destination.generic_string());
        saveProjectCatalog();
        m_assetSelector.discover(*m_assets);
        m_contentBrowser.refresh();
        m_importActivity = "Idle";
        setOperationResult(
            "Imported " + destination.filename().generic_string(),
            OperationTone::Success
        );
        return destination;
    } catch (const std::exception& error) {
        m_importActivity = "Idle";
        setOperationResult("Asset import failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to import asset '{}': {}",
            sourcePath.generic_string(),
            error.what()
        );
        return std::nullopt;
    }
}

void EditorLayer::saveSelectedAsPrefab() {
    if (m_sceneState != SceneState::Edit
        || !m_editorScene
        || !m_editorScene->valid(m_selectedEntity)) {
        return;
    }

    IGFD::FileDialogConfig config;
    if (m_project) {
        config.path = m_project->prefabDirectory().generic_string();
    } else {
        config.path = ".";
    }
    config.fileName = std::string(m_selectedEntity.name()) + ".vsprefab";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_Modal
        | ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog(
        savePrefabDialogKey,
        "Save Prefab As",
        ".vsprefab",
        config
    );
}

void EditorLayer::writePrefab(const std::filesystem::path& path) {
    if (m_sceneState != SceneState::Edit
        || !m_editorScene
        || !m_editorScene->valid(m_selectedEntity)
        || path.empty()) {
        return;
    }

    try {
        std::filesystem::create_directories(path.parent_path());
        const auto prefab = vshade::scene::Prefab::fromEntity(
            *m_editorScene,
            m_selectedEntity
        );
        const auto result = prefab.save(path);
        if (!result) {
            throw std::runtime_error(result.error().message);
        }
        if (m_assets) {
            const auto prefabRef =
                m_assets->reference<vshade::scene::Prefab>(path);
            if (m_assets->isLoaded(prefabRef.handle())) {
                m_assets->unload(prefabRef.handle());
            }
            const auto loaded = m_assets->loadResource<vshade::scene::Prefab>(
                prefabRef
            );
            if (m_editorScene->valid(m_selectedEntity)) {
                if (auto* instance = m_selectedEntity.tryGet<
                        vshade::scene::PrefabInstanceComponent>()) {
                    instance->prefab = loaded.reference();
                } else {
                    m_editorScene->bindPrefabInstance(
                        m_selectedEntity,
                        loaded.reference()
                    );
                }
            }
            m_editorScene->applyPrefabInstances(*m_assets);
            saveProjectCatalog();
            m_assetSelector.discover(*m_assets);
        }
        ENGINE_INFO("Saved prefab '{}'", path.generic_string());
        setOperationResult(
            "Saved prefab " + path.filename().generic_string(),
            OperationTone::Success
        );
    } catch (const std::exception& error) {
        setOperationResult("Prefab save failed", OperationTone::Error);
        ENGINE_ERROR(
            "Failed to save prefab '{}': {}",
            path.generic_string(),
            error.what()
        );
    }
}

void EditorLayer::duplicateSelectedEntity() {
    if (m_sceneState != SceneState::Edit) {
        return;
    }
    if (m_editorScene && m_editorScene->valid(m_selectedEntity)) {
        beginSceneEdit();
        m_selectedEntity = m_editorScene->duplicateEntity(m_selectedEntity);
        commitSceneEdit();
    }
}

void EditorLayer::deleteSelectedEntity() {
    if (m_sceneState != SceneState::Edit) {
        return;
    }
    if (m_editorScene && m_editorScene->valid(m_selectedEntity)) {
        beginSceneEdit();
        m_editorScene->destroyEntity(m_selectedEntity);
        m_selectedEntity = {};
        commitSceneEdit();
    }
}

void EditorLayer::beginSceneEdit() {
    if (m_sceneState != SceneState::Edit || !m_editorScene) {
        return;
    }
    m_undoHistory.begin(*m_editorScene, m_selectedEntity);
}

void EditorLayer::commitSceneEdit() {
    if (!m_editorScene) {
        return;
    }
    if (m_undoHistory.commit(*m_editorScene, m_selectedEntity)) {
        m_document.setCurrentRevision(m_undoHistory.currentRevision());
    }
}

void EditorLayer::revertSceneEdit() {
    if (!m_editorScene) {
        m_undoHistory.cancel();
        return;
    }
    if (m_undoHistory.revert(*m_editorScene, m_selectedEntity)) {
        m_document.setCurrentRevision(m_undoHistory.currentRevision());
    }
}

void EditorLayer::cancelSceneEdit() {
    m_undoHistory.cancel();
}

void EditorLayer::undoSceneEdit() {
    if (m_sceneState != SceneState::Edit || !m_editorScene) {
        return;
    }
    if (m_undoHistory.undo(*m_editorScene, m_selectedEntity)) {
        m_document.setCurrentRevision(m_undoHistory.currentRevision());
    }
}

void EditorLayer::redoSceneEdit() {
    if (m_sceneState != SceneState::Edit || !m_editorScene) {
        return;
    }
    if (m_undoHistory.redo(*m_editorScene, m_selectedEntity)) {
        m_document.setCurrentRevision(m_undoHistory.currentRevision());
    }
}

void EditorLayer::requestTransition(std::function<void()> action) {
    if (!action || m_pendingTransition) {
        return;
    }
    if (!m_document.isDirty()) {
        action();
        return;
    }
    m_pendingTransition = std::move(action);
    m_openUnsavedChangesRequested = true;
}

void EditorLayer::completePendingTransition() {
    std::function<void()> action = std::move(m_pendingTransition);
    m_pendingTransition = {};
    m_openUnsavedChangesRequested = false;
    m_continueAfterSave = false;
    if (action) {
        action();
    }
}

void EditorLayer::cancelPendingTransition() {
    m_pendingTransition = {};
    m_openUnsavedChangesRequested = false;
    m_continueAfterSave = false;
}

void EditorLayer::requestExit() {
    requestTransition([this] {
        if (m_requestExit) {
            m_requestExit();
        }
    });
}

void EditorLayer::handleEditHotkeys() {
    if (m_sceneState != SceneState::Edit || ImGui::GetIO().WantTextInput) {
        return;
    }

    using vshade::input::Input;
    using vshade::input::KeyCode;

    const bool controlDown =
        Input::isKeyDown(KeyCode::LeftControl)
        || Input::isKeyDown(KeyCode::RightControl);
    const bool shiftDown =
        Input::isKeyDown(KeyCode::LeftShift)
        || Input::isKeyDown(KeyCode::RightShift);
    if (controlDown && Input::isKeyPressed(KeyCode::N)) {
        requestTransition([this] { newScene(); });
    } else if (controlDown && Input::isKeyPressed(KeyCode::O)) {
        requestTransition([this] { openScene(); });
    } else if (controlDown && Input::isKeyPressed(KeyCode::S)) {
        if (shiftDown) {
            saveSceneAs();
        } else {
            static_cast<void>(saveScene());
        }
    } else if (controlDown && Input::isKeyPressed(KeyCode::Z)) {
        if (shiftDown) {
            redoSceneEdit();
        } else {
            undoSceneEdit();
        }
    } else if (controlDown && Input::isKeyPressed(KeyCode::Y)) {
        redoSceneEdit();
    } else if (controlDown && Input::isKeyPressed(KeyCode::D)) {
        duplicateSelectedEntity();
    } else if (Input::isKeyPressed(KeyCode::Delete) && !ImGui::IsAnyItemActive()) {
        deleteSelectedEntity();
    }
}

void EditorLayer::recordStartSceneIfUnset() {
    if (!m_project
        || m_document.path().empty()
        || !m_project->config().startScene.empty()) {
        return;
    }

    try {
        m_project->setStartScene(m_document.path());
        if (const auto result = m_project->save(); !result) {
            ENGINE_WARN(
                "Failed to record start scene '{}': {}",
                m_document.path().generic_string(),
                result.error().message
            );
        }
    } catch (const std::exception& error) {
        ENGINE_WARN(
            "Failed to record start scene '{}': {}",
            m_document.path().generic_string(),
            error.what()
        );
    }
}

void EditorLayer::playScene() {
    if (m_sceneState != SceneState::Edit
        || !m_editorScene
        || !m_runtime) {
        return;
    }

    try {
        if (m_assets) {
            beginSceneEdit();
            m_editorScene->applyPrefabInstances(*m_assets);
            commitSceneEdit();
        }
        m_runtimeScene = std::shared_ptr<vshade::scene::Scene>(
            m_editorScene->instantiate()
        );
        m_runtime->play(*m_runtimeScene);
        m_sceneState = SceneState::Play;
        m_stepRequested = false;
        bindActiveScene();
        setOperationResult("Entered Play mode", OperationTone::Success);
    } catch (const std::exception& error) {
        if (m_undoHistory.isRecording()) {
            revertSceneEdit();
        }
        m_runtime->stop();
        m_runtimeScene.reset();
        m_sceneState = SceneState::Edit;
        bindActiveScene();
        setOperationResult("Failed to enter Play mode", OperationTone::Error);
        ENGINE_ERROR("Failed to enter play mode: {}", error.what());
    }
}

void EditorLayer::pauseScene() {
    if (!m_runtime) {
        return;
    }
    if (m_sceneState == SceneState::Play) {
        m_sceneState = SceneState::Pause;
        m_runtime->setPaused(true);
        setOperationResult("Paused scene", OperationTone::Warning);
    } else if (m_sceneState == SceneState::Pause) {
        m_sceneState = SceneState::Play;
        m_stepRequested = false;
        m_runtime->setPaused(false);
        setOperationResult("Resumed scene", OperationTone::Success);
    }
}

void EditorLayer::stepScene() {
    if (m_sceneState == SceneState::Pause) {
        m_stepRequested = true;
    }
}

void EditorLayer::stopScene() {
    if (m_sceneState == SceneState::Edit) {
        return;
    }
    if (m_runtime) {
        m_runtime->stop();
    }
    m_sceneState = SceneState::Edit;
    m_stepRequested = false;
    bindActiveScene();
    m_runtimeScene.reset();
    setOperationResult("Returned to Edit mode", OperationTone::Neutral);
}

void EditorLayer::bindActiveScene() {
    const std::shared_ptr<vshade::scene::Scene>& activeScene =
        m_sceneState == SceneState::Edit
            ? m_editorScene
            : m_runtimeScene;
    m_selectedEntity = {};
    m_sceneHierarchyPanel.setScene(activeScene);
    m_viewport.setScene(activeScene);
}

void EditorLayer::setOperationResult(
    std::string message,
    const OperationTone tone
) {
    m_lastOperation = std::move(message);
    m_lastOperationTone = tone;
    m_toastSecondsRemaining = 3.5F;
}

} // namespace editor
