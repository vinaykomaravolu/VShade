#include "EditorApplication.hpp"
#include "EditorLayer.hpp"

#include <renderer/Renderer.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <ImGuizmo.h>

#include <memory>
#include <stdexcept>

namespace editor {

EditorApplication::EditorApplication()
    : vshade::core::Application({.window = {
          .title = "VShade Editor",
          .width = 1920,
          .height = 1080,
          .fullscreen = false,
          .vsync = true,
      }}) {}

EditorApplication::~EditorApplication() = default;

void EditorApplication::onStart() {
    auto* glfwWindow = window().nativeHandle();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_contextCreated = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "VShadeEditor.ini";
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0F;
    style.WindowBorderSize = 1.0F;

    if (!ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true)) {
        throw std::runtime_error("Dear ImGui GLFW backend initialization failed");
    }
    m_glfwBackendInitialized = true;

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        throw std::runtime_error("Dear ImGui OpenGL backend initialization failed");
    }
    m_openGlBackendInitialized = true;

    vshade::renderer::Renderer::setClearColor({0.08F, 0.09F, 0.11F, 1.0F});

    m_editorLayer = std::make_unique<EditorLayer>(assets(), runtime());
    m_editorLayer->onAttach();
}

void EditorApplication::onUpdate(const float deltaTime) {
    if (m_editorLayer) {
        m_editorLayer->onUpdate(deltaTime);
        window().setCursorCaptured(m_editorLayer->wantsCursorCapture());
    }
}

void EditorApplication::onRender() {
    vshade::renderer::Renderer::clear();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if (m_editorLayer) {
        m_editorLayer->onImGuiRender();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorApplication::onShutdown() noexcept {
    window().setCursorCaptured(false);
    m_editorLayer.reset();

    if (m_openGlBackendInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        m_openGlBackendInitialized = false;
    }
    if (m_glfwBackendInitialized) {
        ImGui_ImplGlfw_Shutdown();
        m_glfwBackendInitialized = false;
    }
    if (m_contextCreated) {
        ImGui::DestroyContext();
        m_contextCreated = false;
    }
}

} // namespace editor
