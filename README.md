# VShade

VShade is a small C++20 OpenGL engine runtime scaffold. It currently provides:

- a cross-platform GLFW window and OpenGL 3.3 context;
- an application lifecycle with clamped variable and fixed-step updates;
- resize callbacks, fullscreen switching, and VSync controls;
- engine and game logging through spdlog;
- assertions and basic filesystem helpers;
- a small `vshade::math` API backed by GLM;
- an OpenGL 3.3 renderer with buffers, shaders, textures, framebuffers, cameras, and meshes;
- high-level 2D quad/sprite and 3D mesh/material rendering;
- a fly-camera controller with cursor capture;
- a static engine library;
- a minimal sandbox application.

The generated [API reference](https://vinaykomaravolu.github.io/VShade/) is
published with GitHub Pages. Documentation source and local build instructions
are in [docs/api.md](docs/api.md). The renderer architecture and lifecycle are
described in [docs/renderer.md](docs/renderer.md). Offscreen golden-image tests
are documented in [docs/visual-testing.md](docs/visual-testing.md).

## Get the dependencies

After cloning the repository, initialize its submodules:

```sh
git submodule update --init --recursive
```

## Build

The same commands work with Visual Studio on Windows, GCC or Clang on Linux,
and Apple Clang on macOS:

```sh
cmake -S . -B build
cmake --build build --config Debug
```

The sandbox renders a lit, textured cube. Use WASD to move, hold the right
mouse button to look, scroll to change speed, and press Escape to exit.

## Test

Tests are enabled by default and use Catch2 with CTest:

```sh
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

For a build that does not compile the tests or Catch2, configure with:

```sh
cmake -S . -B build-no-tests -DBUILD_TESTING=OFF
```

### Coverage

Pushes and pull requests targeting `main` run the coverage workflow on GitHub.
It builds the engine with GCC coverage instrumentation, runs the Catch2 suite,
generates `coverage.xml` with gcovr, and uploads the report to Codecov.

The first upload requires the repository to be enabled in Codecov. The workflow
uses GitHub OIDC, so it does not require a `CODECOV_TOKEN` secret.

## Use the library from another CMake target

When the engine and game are in the same source tree:

```cmake
add_subdirectory(path/to/VShade)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE VShade::Engine)
```

Then include the public API with:

```cpp
#include <core/application.hpp>
#include <core/entrypoint.hpp>
```

A game supplies behavior by deriving from `Application`:

```cpp
class MyGame final : public vshade::core::Application {
protected:
    void onFixedUpdate(float fixedDeltaTime) override {
        // Update fixed-step simulation or physics here.
    }

    void onUpdate(float deltaTime) override {
        // Update the active scene here.
    }
};

SHADE_ENGINE_MAIN(MyGame)
```

Input is stored once per frame, so it does not need a `Window` argument:

```cpp
if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
    close();
}

const glm::vec2 movement = vshade::input::Input::mouseDelta();
```

Use `isKeyDown` for a held key, `isKeyPressed` for its first frame down,
and `isKeyReleased` for its first frame up. Mouse buttons provide the same
three states.

Other core systems have focused headers:

```cpp
#include <core/assert.hpp>
#include <core/entrypoint.hpp>
#include <core/filesystem.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <math/math.hpp>
#include <math/matrix.hpp>
#include <math/quaternion.hpp>
#include <math/transform.hpp>
#include <math/vector.hpp>
#include <input/input.hpp>
#include <input/keycode.hpp>
#include <input/mousecode.hpp>
#include <platform/window.hpp>
#include <renderer/buffer.hpp>
#include <renderer/camera.hpp>
#include <renderer/cameracontroller.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/material.hpp>
#include <renderer/mesh.hpp>
#include <renderer/renderer.hpp>
#include <renderer/renderer2d.hpp>
#include <renderer/renderer3d.hpp>
#include <renderer/rendertypes.hpp>
#include <renderer/shader.hpp>
#include <renderer/texture.hpp>
#include <renderer/vertexarray.hpp>
```

GLAD loads OpenGL 3.3 core functions after the window creates its context. The
renderer is initialized and shut down by `Application`; games submit rendering
work from `onRender()`.
