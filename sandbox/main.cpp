#include <core/application.hpp>
#include <core/entrypoint.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <input/input.hpp>

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
    void onStart() override {
        GAME_INFO("Sandbox started");
    }

    void onUpdate(const float delta_time) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }

        GAME_INFO(
            "Mouse position: {} {}",
            vshade::input::Input::mousePosition().x,
            vshade::input::Input::mousePosition().y
        );

        static_cast<void>(delta_time);
    }

    void onShutdown() override {
        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::frameCount(),
            vshade::core::Time::elapsedTime()
        );
    }
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
