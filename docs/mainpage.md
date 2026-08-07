# VShade Engine API {#mainpage}

VShade is a small C++20 OpenGL game engine runtime for building 2D and 3D
games. The API is intentionally compact while the engine foundation is being
built.

## Start here

- `vshade::core::Application` owns startup, the main loop, and shutdown.
- `SHADE_ENGINE_MAIN` creates the executable entry point for a game application.
- `vshade::platform::Window` owns the GLFW window and OpenGL context.
- `vshade::platform::Input` exposes per-frame keyboard and mouse state.
- `vshade::core::Time` provides frame timing.
- `vshade::core::filesystem` contains basic file-reading helpers.
- `vshade::core::Log` and the logging macros separate engine messages from game messages.

## Minimal application

```cpp
#include <core/application.hpp>
#include <core/entrypoint.hpp>
#include <platform/input.hpp>

class Game final : public vshade::core::Application {
protected:
    void OnUpdate(float delta_time) override {
        if (vshade::platform::Input::is_key_pressed(vshade::platform::KeyCode::Escape)) {
            Close();
        }

        static_cast<void>(delta_time);
    }
};

SHADE_ENGINE_MAIN(Game)
```

Use the Classes, Namespaces, Files, and Topics sections in the navigation panel
to explore the complete public API.
