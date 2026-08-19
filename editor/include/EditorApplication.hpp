#pragma once

#include <core/Application.hpp>

#include <memory>

namespace editor {

class EditorLayer;

class EditorApplication final : public vshade::core::Application {
public:
    EditorApplication();
    ~EditorApplication() override;

private:
    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    void onShutdown() noexcept override;

    std::unique_ptr<EditorLayer> m_editorLayer;
    bool m_contextCreated = false;
    bool m_glfwBackendInitialized = false;
    bool m_openGlBackendInitialized = false;
};

} // namespace editor
