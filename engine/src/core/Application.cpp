#include "core/Application.hpp"

#include "asset/AssetManager.hpp"
#include "audio/AudioService.hpp"
#include "core/EngineServices.hpp"
#include "core/TypeRegistry.hpp"
#include "core/Log.hpp"
#include "core/Time.hpp"
#include "renderer/Renderer.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneRuntime.hpp"
#include "script/NativeScriptRegistry.hpp"

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
    stopScene();
    m_runtime.reset();
    if (m_services) {
        m_services->shutdown();
    }
    m_services.reset();
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

        m_services = std::make_unique<EngineServices>(m_config.audio);
        m_runtime = std::make_unique<scene::SceneRuntime>(*m_services);

        Time::reset();
        double fixedAccumulator = 0.0;
        m_running = true;
        shutdown_needed = true;
        onStart();

        while (m_running && !m_window->shouldClose()) {
            m_window->pollEvents();
            Time::tick(m_config.maximumDeltaTime);
            fixedAccumulator += static_cast<double>(Time::deltaTime());
            while (fixedAccumulator >= static_cast<double>(m_config.fixedDeltaTime)) {
                onFixedUpdate(m_config.fixedDeltaTime);
                if (m_runtime->isPlaying() && !m_runtime->isPaused()) {
                    m_runtime->fixedUpdate(m_config.fixedDeltaTime);
                }
                fixedAccumulator -= static_cast<double>(m_config.fixedDeltaTime);
            }

            onUpdate(Time::deltaTime());
            if (m_runtime->isPlaying() && !m_runtime->isPaused()) {
                m_runtime->update(Time::deltaTime());
            }

            renderer::Renderer::beginFrame();
            if (m_config.presentRuntime && m_runtime->isPlaying()) {
                m_runtime->render(m_window->width(), m_window->height());
            }
            onRender();
            renderer::Renderer::endFrame();

            m_window->swapBuffers();
        }

        stopScene();
        shutdown_needed = false;
        onShutdown();

        m_runtime.reset();
        m_services->shutdown();
        m_services.reset();

        m_running = false;
        ENGINE_INFO("VShade shutdown complete after {} frames", Time::frameCount());
        return 0;
    } catch (...) {
        if (shutdown_needed) {
            stopScene();
            try {
                onShutdown();
            } catch (...) {
                ENGINE_ERROR("Application shutdown hook threw an exception");
            }
        }

        m_runtime.reset();
        if (m_services) {
            m_services->shutdown();
            m_services.reset();
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

platform::Window& Application::window() {
    if (!m_window) {
        throw std::logic_error("Window is only available while the application is running");
    }
    return *m_window;
}

const platform::Window& Application::window() const {
    if (!m_window) {
        throw std::logic_error("Window is only available while the application is running");
    }
    return *m_window;
}

EngineServices& Application::services() {
    if (!m_services) {
        throw std::logic_error("Engine services are only available while the application is running");
    }
    return *m_services;
}

asset::AssetManager& Application::assets() {
    return services().assets();
}

audio::AudioService& Application::audio() {
    return services().audio();
}

script::NativeScriptRegistry& Application::scripts() {
    return services().scripts();
}

TypeRegistry& Application::types() { return services().types(); }

scene::SceneRuntime& Application::runtime() {
    if (!m_runtime) {
        throw std::logic_error("Scene runtime is only available while the application is running");
    }
    return *m_runtime;
}

scene::Scene& Application::playScene(const std::filesystem::path& path) {
    const auto sceneAsset = assets().loadResource<scene::Scene>(path);
    std::unique_ptr<scene::Scene> instance = sceneAsset->instantiate();
    stopScene();
    m_ownedScene = std::move(instance);
    runtime().play(*m_ownedScene);
    return *m_ownedScene;
}

void Application::playScene(scene::Scene& sceneToPlay) {
    stopScene();
    m_ownedScene.reset();
    runtime().play(sceneToPlay);
}

void Application::stopScene() noexcept {
    if (m_runtime) {
        m_runtime->stop();
    }
    m_ownedScene.reset();
}

scene::Scene* Application::activeScene() noexcept {
    return m_runtime ? m_runtime->scene() : nullptr;
}

} // namespace vshade::core
