#include "Core/Application.hpp"

#include "Core/Assert.hpp"
#include "Core/Log.hpp"
#include "Core/Time.hpp"

#include <stdexcept>
#include <utility>

namespace VShade {

Application::Application(ApplicationConfig config)
    : m_config(std::move(config)) {}

Application::~Application() = default;

int Application::Run() {
    if (m_running) {
        throw std::logic_error("Application is already running");
    }

    Log::Initialize();
    bool shutdown_needed = false;

    try {
        ENGINE_INFO("Starting VShade");

        m_window = std::make_unique<Window>(m_config.window);
        m_window->SetResizeCallback([this](const std::uint32_t width, const std::uint32_t height) {
            OnWindowResize(width, height);
        });

        Time::Reset();
        m_running = true;
        OnStart();
        shutdown_needed = true;

        while (m_running && !m_window->ShouldClose()) {
            m_window->PollEvents();
            Time::Tick();

            // Later: accumulate delta time and call a fixed update at 1 / 60
            // seconds for deterministic physics and other fixed-step systems.

            // The application will forward these calls to its active scene
            // once the scene system exists.
            OnUpdate(Time::DeltaTime());

            // Later: render the active scene here once the renderer and scene
            // systems exist.

            m_window->SwapBuffers();
        }

        shutdown_needed = false;
        OnShutdown();

        m_running = false;
        m_window.reset();
        ENGINE_INFO("VShade shutdown complete after {} frames", Time::FrameCount());
        Log::Shutdown();
        return 0;
    } catch (...) {
        if (shutdown_needed) {
            shutdown_needed = false;
            try {
                OnShutdown();
            } catch (...) {
                ENGINE_ERROR("Application shutdown hook threw an exception");
            }
        }

        m_running = false;
        m_window.reset();
        ENGINE_ERROR("VShade stopped because of an unhandled exception");
        Log::Shutdown();
        throw;
    }
}

void Application::Close() {
    m_running = false;
    if (m_window) {
        m_window->RequestClose();
    }
}

Window& Application::GetWindow() {
    ENGINE_ASSERT(m_window != nullptr, "Window is only available while the application is running");
    return *m_window;
}

const Window& Application::GetWindow() const {
    ENGINE_ASSERT(m_window != nullptr, "Window is only available while the application is running");
    return *m_window;
}

} // namespace VShade
