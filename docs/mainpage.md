# VShade Engine API {#mainpage}

VShade is a small C++20 OpenGL game engine runtime for building 2D and 3D
games. The API is intentionally compact while the engine foundation is being
built.

## Start here

- `VShade::Application` owns startup, the main loop, and shutdown.
- `SHADE_ENGINE_MAIN` creates the executable entry point for a game application.
- `VShade::Window` owns the GLFW window and OpenGL context.
- `VShade::Input` exposes per-frame keyboard and mouse state.
- `VShade::Time` provides frame timing.
- `VShade::FileSystem` contains basic file-reading helpers.
- `VShade::Log` and the logging macros separate engine messages from game messages.

## Minimal application

```cpp
#include <Core/Application.hpp>
#include <Core/EntryPoint.hpp>
#include <Platform/Input.hpp>

class Game final : public VShade::Application {
protected:
    void OnUpdate(float delta_time) override {
        if (VShade::Input::is_key_pressed(VShade::KeyCode::Escape)) {
            Close();
        }

        static_cast<void>(delta_time);
    }
};

SHADE_ENGINE_MAIN(Game)
```

Use the Classes, Namespaces, Files, and Topics sections in the navigation panel
to explore the complete public API.
