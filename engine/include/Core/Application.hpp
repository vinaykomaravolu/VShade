#pragma once

#include "Platform/Window.hpp"

#include <memory>

namespace VShade {

struct ApplicationConfig {
    WindowConfig window{};
};

class Application {
public:
    explicit Application(ApplicationConfig config = {});
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    int Run();
    void Close();

    [[nodiscard]] Window& GetWindow();
    [[nodiscard]] const Window& GetWindow() const;

protected:
    virtual void OnStart() {}

    // The future scene system will update the active scene from this hook.
    virtual void OnUpdate(float delta_time) {
        static_cast<void>(delta_time);
    }

    // The future renderer will render the active scene from this hook.
    virtual void OnShutdown() {}

    virtual void OnWindowResize(std::uint32_t width, std::uint32_t height) {
        static_cast<void>(width);
        static_cast<void>(height);
    }

private:
    ApplicationConfig m_config;
    std::unique_ptr<Window> m_window;
    bool m_running = false;
};

} // namespace VShade
