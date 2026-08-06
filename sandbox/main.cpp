#include <Core/Application.hpp>
#include <Core/EntryPoint.hpp>
#include <Core/Log.hpp>
#include <Core/Time.hpp>

namespace {

class SandboxApplication final : public VShade::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade Sandbox",
                  .width = 1920,
                  .height = 1080,
                  .fullscreen = false,
                  .vsync = true,
              },
          }) {}

protected:
    void OnStart() override {
        GAME_INFO("Sandbox started");
    }

    void OnUpdate(const float delta_time) override {
        if (GetWindow().IsKeyPressed(VShade::Key::Escape)) {
            Close();
        }
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

SHADE_ENGINE_MAIN(SandboxApplication)
