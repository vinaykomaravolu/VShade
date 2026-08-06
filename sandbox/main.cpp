#include <Core/Application.hpp>
#include <Core/EntryPoint.hpp>
#include <Core/Log.hpp>
#include <Core/Time.hpp>
#include <Platform/Input.hpp>

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
        if (VShade::Input::is_key_pressed(VShade::KeyCode::Escape)) {
            Close();
        }

        GAME_INFO("Mouse position: {} {}", VShade::Input::mouse_position().x, VShade::Input::mouse_position().y);

        static_cast<void>(delta_time);
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
