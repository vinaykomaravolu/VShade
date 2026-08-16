#include <core/Application.hpp>
#include <core/EntryPoint.hpp>
#include <renderer/Renderer.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <stdexcept>
#include <GLFW/glfw3.h>

namespace {

class EditorApplication final : public vshade::core::Application {
public:
    EditorApplication()
        : Application({.window = {
              .title = "VShade Editor",
              .width = 1920,
              .height = 1080,
              .fullscreen = false,
              .vsync = true,
          }}) {}

protected:
    void onStart() override {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        m_contextCreated = true;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(window().nativeHandle(), true)) {
            throw std::runtime_error("Dear ImGui GLFW backend initialization failed");
        }
        m_glfwBackendInitialized = true;

        if (!ImGui_ImplOpenGL3_Init("#version 330")) {
            throw std::runtime_error("Dear ImGui OpenGL backend initialization failed");
        }
        m_openGlBackendInitialized = true;

        vshade::renderer::Renderer::setClearColor({0.08F, 0.09F, 0.11F, 1.0F});
    }

    void onRender() override {
        vshade::renderer::Renderer::clear();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void onShutdown() override {
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

private:
    bool m_contextCreated = false;
    bool m_glfwBackendInitialized = false;
    bool m_openGlBackendInitialized = false;
};

} // namespace

SHADE_ENGINE_MAIN(EditorApplication)
