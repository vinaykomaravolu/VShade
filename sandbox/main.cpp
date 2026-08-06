#include <exception>
#include <iostream>

#include <Core/Application.hpp>
#include <Core/Log.hpp>
#include <Core/Time.hpp>

namespace {

class SandboxApplication final : public VShade::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade Sandbox",
                  .width = 1280,
                  .height = 720,
                  .fullscreen = false,
                  .vsync = true,
              },
          }) {}

protected:
    void OnStartup() override {
        GAME_INFO("Sandbox started");
    }

    void OnUpdate(const float delta_time) override {
        if (GetWindow().IsKeyPressed(VShade::Key::Escape)) {
            Close();
        }

        // Later: update the active scene here.
        static_cast<void>(delta_time);
    }

    void OnRender() override {
        // Later: render the active scene here.
        GetWindow().Clear(0.06F, 0.07F, 0.10F);
    }

    void OnShutdown() override {
        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            VShade::Time::FrameCount(),
            VShade::Time::ElapsedTime()
        );
    }
};

} // namespace

int main() {
    try {
        SandboxApplication application;
        return application.Run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
