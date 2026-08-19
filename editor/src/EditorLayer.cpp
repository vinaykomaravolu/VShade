#include "EditorLayer.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <core/Log.hpp>
#include <project/Project.hpp>
#include <renderer/Model.hpp>
#include <renderer/Texture.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneRuntime.hpp>
#include <scene/SceneSerializer.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include <ImGuiFileDialog.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace editor {
namespace {

constexpr const char* openSceneDialogKey = "OpenSceneDialog";
constexpr const char* saveSceneDialogKey = "SaveSceneDialog";
constexpr const char* importAssetDialogKey = "ImportAssetDialog";
constexpr const char* openProjectDialogKey = "OpenProjectDialog";
constexpr const char* newProjectDialogKey = "NewProjectDialog";

[[nodiscard]] std::string lowercase(std::string value) {
    std::ranges::transform(
        value,
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );
    return value;
}

enum class ToolbarIcon {
    Play,
    Pause,
    Step,
    Stop,
};

bool toolbarIconButton(
    const char* id,
    const ToolbarIcon icon,
    const ImVec2 size,
    const char* tooltip
) {
    const bool clicked = ImGui::Button(id, size);
    const ImVec2 minimum = ImGui::GetItemRectMin();
    const ImVec2 maximum = ImGui::GetItemRectMax();
    const ImVec2 center{
        (minimum.x + maximum.x) * 0.5F,
        (minimum.y + maximum.y) * 0.5F,
    };
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    switch (icon) {
        case ToolbarIcon::Play:
            drawList->AddTriangleFilled(
                {center.x - 5.0F, center.y - 7.0F},
                {center.x - 5.0F, center.y + 7.0F},
                {center.x + 7.0F, center.y},
                color
            );
            break;
        case ToolbarIcon::Pause:
            drawList->AddRectFilled(
                {center.x - 6.0F, center.y - 7.0F},
                {center.x - 2.0F, center.y + 7.0F},
                color
            );
            drawList->AddRectFilled(
                {center.x + 2.0F, center.y - 7.0F},
                {center.x + 6.0F, center.y + 7.0F},
                color
            );
            break;
        case ToolbarIcon::Step:
            drawList->AddTriangleFilled(
                {center.x - 7.0F, center.y - 7.0F},
                {center.x - 7.0F, center.y + 7.0F},
                {center.x + 4.0F, center.y},
                color
            );
            drawList->AddRectFilled(
                {center.x + 5.0F, center.y - 7.0F},
                {center.x + 8.0F, center.y + 7.0F},
                color
            );
            break;
        case ToolbarIcon::Stop:
            drawList->AddRectFilled(
                {center.x - 6.0F, center.y - 6.0F},
                {center.x + 6.0F, center.y + 6.0F},
                color
            );
            break;
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

} // namespace

EditorLayer::EditorLayer(
    vshade::asset::AssetManager& assets,
    vshade::scene::SceneRuntime& runtime,
    std::function<void()> requestExit
)
    : m_assets(&assets),
      m_runtime(&runtime),
      m_requestExit(std::move(requestExit)),
      m_contentBrowser(std::filesystem::path{}),
      m_viewport(assets) {
    m_inspectorPanel.setAssetManager(assets);
}

void EditorLayer::onAttach() {
    newScene();
}

void EditorLayer::onUpdate(const float deltaTime) {
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
    DrawDockspace();
}

bool EditorLayer::wantsCursorCapture() const noexcept {
    return m_viewport.wantsCursorCapture();
}

void EditorLayer::DrawDockspace()
{
    DrawMenuBar();
    DrawToolbar();

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

    if (m_showHierarchy) {
        m_sceneHierarchyPanel.onImGuiRender(m_selectedEntity);
    }
    m_viewport.setVisible(m_showViewport);
    if (m_showViewport) {
        m_viewport.onImGuiRender(m_selectedEntity);
    }
    if (m_showInspector) {
        m_inspectorPanel.onImGuiRender(m_selectedEntity);
    }

    if (m_showContentBrowser) {
        if (const auto scenePath = m_contentBrowser.onImGuiRender();
            scenePath && m_sceneState == SceneState::Edit) {
            loadScene(*scenePath);
        }
    }
    if (m_showConsole) {
        m_console.onImGuiRender();
    }
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
    ImGui::DockBuilderDockWindow("Content Browser", consoleDockId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
        if (ImGui::MenuItem("New Scene")) {
            newScene();
        }
        if (ImGui::MenuItem("Open Scene...")) {
            openScene();
        }
        if (ImGui::MenuItem("Save Scene")) {
            saveScene();
        }
        if (ImGui::MenuItem("Save Scene As...")) {
            saveSceneAs();
        }
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
                ".glb,.gltf,.png,.jpg,.jpeg,.bmp,.tga,.wav,.mp3,.flac,.ogg",
                config
            );
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
        if (ImGui::MenuItem("Open Project...")) {
            openProject();
        }
        if (ImGui::MenuItem("New Project...")) {
            newProject();
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit") && m_requestExit) {
            m_requestExit();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        const std::shared_ptr<vshade::scene::Scene>& activeScene =
            m_sceneState == SceneState::Edit
                ? m_editorScene
                : m_runtimeScene;
        const bool hasSelection =
            activeScene && activeScene->valid(m_selectedEntity);
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::MenuItem("Duplicate Entity")) {
            duplicateSelectedEntity();
        }
        if (ImGui::MenuItem("Delete Entity")) {
            deleteSelectedEntity();
        }
        ImGui::EndDisabled();
        ImGui::EndMenu();
    }
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

    const float titleWidth = ImGui::CalcTextSize("VShade").x;
    ImGui::SetCursorPosX(
        std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - titleWidth - 12.0F)
    );
    ImGui::TextUnformatted("VShade");
    ImGui::EndMainMenuBar();
}

void EditorLayer::DrawToolbar() {
    constexpr float toolbarHeight = 38.0F;
    constexpr ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::BeginViewportSideBar(
            "##VShadeToolbar",
            ImGui::GetMainViewport(),
            ImGuiDir_Up,
            toolbarHeight,
            toolbarFlags
        )) {
        return;
    }

    constexpr float buttonWidth = 42.0F;
    constexpr float spacing = 8.0F;
    constexpr float totalWidth = buttonWidth * 4.0F + spacing * 3.0F;
    ImGui::SetCursorPosX(
        std::max(0.0F, (ImGui::GetWindowWidth() - totalWidth) * 0.5F)
    );
    ImGui::SetCursorPosY(5.0F);

    ImGui::BeginDisabled(m_sceneState != SceneState::Edit);
    if (toolbarIconButton(
            "##Play",
            ToolbarIcon::Play,
            {buttonWidth, 28.0F},
            "Play"
        )) {
        playScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState == SceneState::Edit);
    const bool paused = m_sceneState == SceneState::Pause;
    if (toolbarIconButton(
            "##Pause",
            paused ? ToolbarIcon::Play : ToolbarIcon::Pause,
            {buttonWidth, 28.0F},
            paused ? "Resume" : "Pause"
        )) {
        pauseScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState != SceneState::Pause);
    if (toolbarIconButton(
            "##Step",
            ToolbarIcon::Step,
            {buttonWidth, 28.0F},
            "Step one frame"
        )) {
        stepScene();
    }
    ImGui::EndDisabled();

    ImGui::SameLine(0.0F, spacing);
    ImGui::BeginDisabled(m_sceneState == SceneState::Edit);
    if (toolbarIconButton(
            "##Stop",
            ToolbarIcon::Stop,
            {buttonWidth, 28.0F},
            "Stop"
        )) {
        stopScene();
    }
    ImGui::EndDisabled();

    ImGui::End();
}

void EditorLayer::DrawFileDialogs() {
    if (ImGuiFileDialog::Instance()->Display(openSceneDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            loadScene(selectedPath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(saveSceneDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            m_activeScenePath = ImGuiFileDialog::Instance()->GetFilePathName(
                IGFD_ResultMode_OverwriteFileExt
            );
            saveScene();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(importAssetDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            importAsset(selectedPath);
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display(openProjectDialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::filesystem::path selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                setProject(vshade::project::Project::load(selectedPath));
            } catch (const std::exception& error) {
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
                setProject(vshade::project::Project::create(
                    selectedDirectory
                ));
            } catch (const std::exception& error) {
                ENGINE_ERROR(
                    "Failed to create project '{}': {}",
                    selectedDirectory.generic_string(),
                    error.what()
                );
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void EditorLayer::setActiveScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    if (!scene) {
        return;
    }
    m_editorScene = std::move(scene);
    bindActiveScene();
}

void EditorLayer::newScene() {
    setActiveScene(
        std::make_shared<vshade::scene::Scene>("Untitled Scene")
    );
    m_activeScenePath.clear();
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

void EditorLayer::loadScene(const std::filesystem::path& path) {
    if (m_sceneState != SceneState::Edit) {
        return;
    }

    auto scene = std::make_shared<vshade::scene::Scene>(
        path.stem().string()
    );
    vshade::scene::SceneSerializer serializer(*scene);
    if (!serializer.deserialize(path)) {
        ENGINE_ERROR(
            "Failed to open scene '{}': {}",
            path.generic_string(),
            serializer.lastError()
        );
        return;
    }

    setActiveScene(std::move(scene));
    m_activeScenePath = path;
}

void EditorLayer::saveScene() {
    if (!m_editorScene) {
        return;
    }
    if (m_activeScenePath.empty()) {
        saveSceneAs();
        return;
    }

    vshade::scene::SceneSerializer serializer(*m_editorScene);
    if (!serializer.serialize(m_activeScenePath, vshade::scene::SceneJsonFormat::Compact)) {
        ENGINE_ERROR(
            "Failed to save scene '{}': {}",
            m_activeScenePath.generic_string(),
            serializer.lastError()
        );
        return;
    }
    ENGINE_INFO("Saved scene '{}'", m_activeScenePath.generic_string());
    recordStartSceneIfUnset();
}

void EditorLayer::saveSceneAs() {
    if (!m_editorScene) {
        return;
    }

    IGFD::FileDialogConfig config;
    if (!m_activeScenePath.empty()) {
        config.path = m_activeScenePath.parent_path().generic_string();
        config.fileName = m_activeScenePath.filename().generic_string();
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

void EditorLayer::setProject(
    std::shared_ptr<vshade::project::Project> project
) {
    if (!project) {
        return;
    }
    m_project = std::move(project);
    m_contentBrowser.setRoot(m_project->assetDirectory());
    AssetSelector::setSearchDirectory(m_project->assetDirectory());
    if (m_assets) {
        AssetSelector::discover(*m_assets);
    }
    ENGINE_INFO(
        "Opened project '{}'",
        m_project->projectFile().generic_string()
    );
    const std::filesystem::path startScene =
        m_project->startScenePath();
    if (!startScene.empty()) {
        if (std::filesystem::is_regular_file(startScene)) {
            loadScene(startScene);
        } else {
            ENGINE_WARN(
                "Project start scene does not exist: '{}'",
                startScene.generic_string()
            );
        }
    }
}

void EditorLayer::importAsset(
    const std::filesystem::path& sourcePath
) {
    if (!m_project || !m_assets || sourcePath.empty()) {
        return;
    }

    try {
        const std::filesystem::path destination =
            m_project->assetDirectory() / sourcePath.filename();
        std::error_code error;
        const bool destinationExists =
            std::filesystem::exists(destination, error);
        if (error) {
            throw std::runtime_error("Unable to inspect the asset destination");
        }
        if (destinationExists) {
            error.clear();
            if (!std::filesystem::equivalent(sourcePath, destination, error)
                || error) {
                throw std::runtime_error(
                    "An asset with this filename already exists"
                );
            }
        } else {
            std::filesystem::copy_file(sourcePath, destination);
        }

        const std::string extension = lowercase(
            destination.extension().generic_string()
        );
        if (extension == ".glb" || extension == ".gltf") {
            static_cast<void>(
                m_assets->load<vshade::renderer::Model>(destination)
            );
        } else if (
            extension == ".png"
            || extension == ".jpg"
            || extension == ".jpeg"
            || extension == ".bmp"
            || extension == ".tga"
        ) {
            static_cast<void>(
                m_assets->load<vshade::renderer::Texture2D>(destination)
            );
        } else if (
            extension == ".wav"
            || extension == ".mp3"
            || extension == ".flac"
            || extension == ".ogg"
        ) {
            static_cast<void>(
                m_assets->load<vshade::audio::AudioClip>(destination)
            );
        } else {
            throw std::invalid_argument("Unsupported asset file type");
        }
        ENGINE_INFO("Imported asset '{}'", destination.generic_string());
        AssetSelector::discover(*m_assets);
    } catch (const std::exception& error) {
        ENGINE_ERROR(
            "Failed to import asset '{}': {}",
            sourcePath.generic_string(),
            error.what()
        );
    }
}

void EditorLayer::duplicateSelectedEntity() {
    const std::shared_ptr<vshade::scene::Scene>& activeScene =
        m_sceneState == SceneState::Edit
            ? m_editorScene
            : m_runtimeScene;
    if (activeScene && activeScene->valid(m_selectedEntity)) {
        m_selectedEntity =
            activeScene->duplicateEntity(m_selectedEntity);
    }
}

void EditorLayer::deleteSelectedEntity() {
    const std::shared_ptr<vshade::scene::Scene>& activeScene =
        m_sceneState == SceneState::Edit
            ? m_editorScene
            : m_runtimeScene;
    if (activeScene && activeScene->valid(m_selectedEntity)) {
        activeScene->destroyEntity(m_selectedEntity);
        m_selectedEntity = {};
    }
}

void EditorLayer::recordStartSceneIfUnset() {
    if (!m_project
        || m_activeScenePath.empty()
        || !m_project->config().startScene.empty()) {
        return;
    }

    try {
        m_project->setStartScene(m_activeScenePath);
        if (const auto result = m_project->save(); !result) {
            ENGINE_WARN(
                "Failed to record start scene '{}': {}",
                m_activeScenePath.generic_string(),
                result.error().message
            );
        }
    } catch (const std::exception& error) {
        ENGINE_WARN(
            "Failed to record start scene '{}': {}",
            m_activeScenePath.generic_string(),
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
        m_runtimeScene = std::shared_ptr<vshade::scene::Scene>(
            m_editorScene->instantiate()
        );
        m_runtime->play(*m_runtimeScene);
        m_sceneState = SceneState::Play;
        m_stepRequested = false;
        bindActiveScene();
    } catch (const std::exception& error) {
        m_runtime->stop();
        m_runtimeScene.reset();
        m_sceneState = SceneState::Edit;
        bindActiveScene();
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
    } else if (m_sceneState == SceneState::Pause) {
        m_sceneState = SceneState::Play;
        m_stepRequested = false;
        m_runtime->setPaused(false);
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

} // namespace editor
