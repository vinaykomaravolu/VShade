#include <core/application.hpp>
#include <core/entrypoint.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <platform/input.hpp>

namespace {

class SandboxApplication final : public vshade::core::Application {
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
        if (vshade::platform::Input::is_key_pressed(vshade::platform::KeyCode::Escape)) {
            Close();
        }

        GAME_INFO(
            "Mouse position: {} {}",
            vshade::platform::Input::mouse_position().x,
            vshade::platform::Input::mouse_position().y
        );

        static_cast<void>(delta_time);
    }

    void OnShutdown() override {
        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::FrameCount(),
            vshade::core::Time::ElapsedTime()
        );
    }
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
