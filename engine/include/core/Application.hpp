#pragma once

#include "audio/AudioTypes.hpp"
#include "platform/Window.hpp"

#include <filesystem>
#include <memory>

namespace vshade::asset { class AssetManager; }
namespace vshade::audio { class AudioService; }
namespace vshade::scene { class Scene; class SceneRuntime; }
namespace vshade::script { class NativeScriptRegistry; }

namespace vshade::core {

class EngineServices;
class TypeRegistry;

/** @brief Configuration used to start an Application. */
struct ApplicationConfig {
    /** @brief Settings used to create the main window. */
    platform::WindowConfig window{};
    /** @brief Fixed-update interval in seconds. */
    float fixedDeltaTime = 1.0F / 60.0F;
    /** @brief Maximum variable frame delta accepted after a stall. */
    float maximumDeltaTime = 0.25F;
    /** @brief Settings used if the lazily initialized audio service is requested. */
    audio::AudioEngineConfig audio{};
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
    /**
     * @brief Creates an application with the supplied startup configuration.
     * @param config Window and engine startup settings.
     */
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

    /**
     * @brief Returns the main window while the application is running.
     * @return Mutable access to the application-owned window.
     * @warning Call only after window creation and before application shutdown.
     */
    [[deprecated("Use window()")]] [[nodiscard]] platform::Window& getWindow();

    /**
     * @brief Returns the main window while the application is running.
     * @return Read-only access to the application-owned window.
     * @warning Call only after window creation and before application shutdown.
     */
    [[deprecated("Use window()")]] [[nodiscard]] const platform::Window& getWindow() const;

    /** @brief Preferred concise alias for getWindow(). */
    [[nodiscard]] platform::Window& window();

    /** @brief Preferred concise alias for getWindow(). */
    [[nodiscard]] const platform::Window& window() const;

protected:
    /** @brief Returns the application-owned asset service while running. */
    [[nodiscard]] asset::AssetManager& assets();

    /** @brief Returns the lazily initialized application audio service. */
    [[nodiscard]] audio::AudioService& audio();

    /** @brief Returns the application-owned native script type registry. */
    [[nodiscard]] script::NativeScriptRegistry& scripts();

    /** @brief Registers components and scripts through one startup surface. */
    [[nodiscard]] TypeRegistry& types();

    /** @brief Returns the application-owned scene runtime while running. */
    [[nodiscard]] scene::SceneRuntime& runtime();

    /** @brief Instantiates and starts a serialized scene asset. */
    [[nodiscard]] scene::Scene& playScene(const std::filesystem::path& path);

    /** @brief Starts an externally owned scene, which must outlive play mode. */
    void playScene(scene::Scene& scene);

    /** @brief Stops the active scene, if any. */
    void stopScene() noexcept;

    /** @brief Returns the active scene or null when no scene is playing. */
    [[nodiscard]] scene::Scene* activeScene() noexcept;

    /** @brief Returns all application-lifetime services while running. */
    [[nodiscard]] EngineServices& services();

    /** @brief Called once after the window and time system are ready. */
    virtual void onStart() {}

    /**
     * @brief Called once per frame for game and scene updates.
     * @param delta_time Seconds elapsed since the previous frame.
     */
    virtual void onUpdate(float delta_time) {
        static_cast<void>(delta_time);
    }

    /**
     * @brief Called zero or more times per frame at a stable simulation interval.
     * @param fixed_delta_time Configured fixed timestep in seconds.
     */
    virtual void onFixedUpdate(float fixed_delta_time) {
        static_cast<void>(fixed_delta_time);
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
    std::unique_ptr<EngineServices> m_services;
    std::unique_ptr<scene::SceneRuntime> m_runtime;
    std::unique_ptr<scene::Scene> m_ownedScene;
    bool m_running = false;
};

} // namespace vshade::core