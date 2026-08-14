#include "core/Application.hpp"

#include "core/Log.hpp"
#include "core/Time.hpp"
#include "renderer/Renderer.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace vshade::core {

Application::Application(ApplicationConfig config)
    : m_config(std::move(config)) {
    if (!std::isfinite(m_config.fixedDeltaTime) || m_config.fixedDeltaTime <= 0.0F) {
        throw std::invalid_argument("Application fixed delta time must be finite and positive");
    }
    if (!std::isfinite(m_config.maximumDeltaTime) || m_config.maximumDeltaTime <= 0.0F) {
        throw std::invalid_argument("Application maximum delta time must be finite and positive");
    }
}

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
        renderer::Renderer::setViewport(0, 0, m_window->width(), m_window->height());
        m_window->setResizeCallback([this](const std::uint32_t width, const std::uint32_t height) {
            renderer::Renderer::setViewport(0, 0, width, height);
            onWindowResize(width, height);
        });

        Time::reset();
        double fixedAccumulator = 0.0;
        m_running = true;
        onStart();
        shutdown_needed = true;

        while (m_running && !m_window->shouldClose()) {
            m_window->pollEvents();
            Time::tick(m_config.maximumDeltaTime);
            fixedAccumulator += static_cast<double>(Time::deltaTime());
            while (fixedAccumulator >= static_cast<double>(m_config.fixedDeltaTime)) {
                onFixedUpdate(m_config.fixedDeltaTime);
                fixedAccumulator -= static_cast<double>(m_config.fixedDeltaTime);
            }

            // The application will forward these calls to its active scene
            // once the scene system exists.
            onUpdate(Time::deltaTime());

            renderer::Renderer::beginFrame();
            onRender();
            renderer::Renderer::endFrame();

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
    if (!m_window) {
        throw std::logic_error("Window is only available while the application is running");
    }
    return *m_window;
}

const platform::Window& Application::getWindow() const {
    if (!m_window) {
        throw std::logic_error("Window is only available while the application is running");
    }
    return *m_window;
}

} // namespace vshade::core
