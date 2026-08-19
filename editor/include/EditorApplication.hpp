#pragma once

#include <core/Application.hpp>

namespace editor {

class EditorApplication final : public vshade::core::Application {
public:
    EditorApplication();
    ~EditorApplication() override = default;

private:
    void onStart() override;
    void onRender() override;
    void onShutdown() noexcept override;

    bool m_contextCreated = false;
    bool m_glfwBackendInitialized = false;
    bool m_openGlBackendInitialized = false;
};

} // namespace editor
