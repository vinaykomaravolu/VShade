# VShade Engine API {#mainpage}

VShade is a small C++20 OpenGL game engine runtime for building 2D and 3D
games. The API is intentionally compact while the engine foundation is being
built.

## Start here

- `vshade::core::Application` owns startup, the main loop, and shutdown.
- `SHADE_ENGINE_MAIN` creates the executable entry point for a game application.
- `vshade::platform::Window` owns the GLFW window and OpenGL context.
- `vshade::input::Input` exposes per-frame keyboard and mouse state.
- `vshade::core::Time` provides clamped frame timing; `Application` also exposes fixed updates.
- `vshade::core::filesystem` contains basic file-reading helpers.
- `vshade::core::Log` and the logging macros separate engine messages from game messages.
- `vshade::math` provides GLM-backed vectors, matrices, quaternions, and transforms.
- `vshade::renderer::Renderer` provides OpenGL startup, tracked pipeline state, and drawing.
- `vshade::renderer::Renderer2D` submits quads and sprites.
- `vshade::renderer::Renderer3D` submits meshes, materials, cameras, and directional lighting.
- Renderer resource classes own buffers, vertex arrays, shaders, textures, framebuffers, and meshes.
- `vshade::renderer::CameraController` provides testable fly-camera behavior.
- `vshade::scene::Scene` owns entities and serializable components through EnTT.

See [Renderer foundation](renderer.md) for the renderer architecture,
lifecycle, file responsibilities, and current scope.

See [Scenes and serialization](scene.md) for entities, stable UUIDs,
duplication, asset paths, and the JSON scene format.

See [Golden-image renderer testing](visual-testing.md) for hidden-context
visual regression tests, comparison tolerances, and failure artifacts.

## Minimal application

```cpp
#include <core/Application.hpp>
#include <core/EntryPoint.hpp>
#include <input/Input.hpp>

class Game final : public vshade::core::Application {
protected:
    void onUpdate(float delta_time) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }

        static_cast<void>(delta_time);
    }
};

SHADE_ENGINE_MAIN(Game)
```

## API indexes

- [Class list](annotated.html)
- [Alphabetical class index](classes.html)
- [Namespaces](namespaces.html)
- [Files](files.html)
- [Topics](topics.html)

These direct links also work when JavaScript is unavailable. The same indexes
remain available from the navigation panel.
