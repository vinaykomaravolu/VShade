#pragma once

#include "platform/window.hpp"

#include <memory>

namespace vshade::core {

/** @brief Configuration used to start an Application. */
struct ApplicationConfig {
    /** @brief Settings used to create the main window. */
    platform::WindowConfig window{};
};

/**
 * @brief Owns the engine lifecycle and main window.
 *
 * Derive from Application and override the lifecycle hooks to implement a
 * game. Use SHADE_ENGINE_MAIN with the derived type to create the program's
 * entry point.
 */
class Application {
public:
    /** @brief Creates an application with the supplied startup configuration. */
    explicit Application(ApplicationConfig config = {});

    /** @brief Releases application resources. */
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief Starts the engine, runs frames until closed, and shuts it down.
     * @return Zero after a normal shutdown.
     * @throws std::exception If startup or a lifecycle hook fails.
     */
    int run();

    /** @brief Requests that the main loop stop after the current frame. */
    void close();

    /** @brief Returns the main window while the application is running. */
    [[nodiscard]] platform::Window& getWindow();

    /** @brief Returns the main window while the application is running. */
    [[nodiscard]] const platform::Window& getWindow() const;

protected:
    /** @brief Called once after the window and time system are ready. */
    virtual void onStart() {}

    /**
     * @brief Called once per frame for game and scene updates.
     * @param delta_time Seconds elapsed since the previous frame.
     */
    virtual void onUpdate(float delta_time) {
        static_cast<void>(delta_time);
    }

    /** @brief Called once per frame to submit and execute rendering work. */
    virtual void onRender() {}

    /** @brief Called once before application resources are released. */
    virtual void onShutdown() {}

    /**
     * @brief Called when the framebuffer size changes.
     * @param width New framebuffer width in pixels.
     * @param height New framebuffer height in pixels.
     */
    virtual void onWindowResize(std::uint32_t width, std::uint32_t height) {
        static_cast<void>(width);
        static_cast<void>(height);
    }

private:
    ApplicationConfig m_config;
    std::unique_ptr<platform::Window> m_window;
    bool m_running = false;
};

} // namespace vshade::core
