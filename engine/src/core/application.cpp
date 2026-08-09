#include "core/application.hpp"

#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/time.hpp"
#include "renderer/renderer.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::core {

Application::Application(ApplicationConfig config)
    : m_config(std::move(config)) {}

Application::~Application() {
    renderer::Renderer::shutdown();
    m_window.reset();
    Log::shutdown();
}

int Application::run() {
    if (m_running) {
        throw std::logic_error("Application is already running");
    }

    Log::initialize();
    bool shutdown_needed = false;
    try {
        ENGINE_INFO("Starting VShade");

        m_window = std::make_unique<platform::Window>(m_config.window);
        renderer::Renderer::initialize();
        renderer::Renderer::setViewport(m_window->width(), m_window->height());
        m_window->setResizeCallback([this](const std::uint32_t width, const std::uint32_t height) {
            renderer::Renderer::setViewport(width, height);
            onWindowResize(width, height);
        });

        Time::reset();
        m_running = true;
        onStart();
        shutdown_needed = true;

        while (m_running && !m_window->shouldClose()) {
            m_window->pollEvents();
            Time::tick();

            // Later: accumulate delta time and call a fixed update at 1 / 60
            // seconds for deterministic physics and other fixed-step systems.

            // The application will forward these calls to its active scene
            // once the scene system exists.
            onUpdate(Time::deltaTime());

            renderer::Renderer::beginFrame();
            onRender();

            m_window->swapBuffers();
        }

        shutdown_needed = false;
        onShutdown();

        m_running = false;
        ENGINE_INFO("VShade shutdown complete after {} frames", Time::frameCount());
        return 0;
    } catch (...) {
        if (shutdown_needed) {
            shutdown_needed = false;
            try {
                onShutdown();
            } catch (...) {
                ENGINE_ERROR("Application shutdown hook threw an exception");
            }
        }

        m_running = false;
        ENGINE_ERROR("VShade stopped because of an unhandled exception");
        throw;
    }
}

void Application::close() {
    m_running = false;
    if (m_window) {
        m_window->requestClose();
    }
}

platform::Window& Application::getWindow() {
    ENGINE_ASSERT(m_window != nullptr, "Window is only available while the application is running");
    return *m_window;
}

const platform::Window& Application::getWindow() const {
    ENGINE_ASSERT(m_window != nullptr, "Window is only available while the application is running");
    return *m_window;
}

} // namespace vshade::core
