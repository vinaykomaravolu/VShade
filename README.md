# VShade

VShade is a small C++20 OpenGL engine runtime scaffold. It currently provides:

- a cross-platform GLFW window and OpenGL 3.3 context;
- an application lifecycle and frame-timed main loop;
- resize callbacks, fullscreen switching, and VSync controls;
- engine and game logging through spdlog;
- assertions and basic filesystem helpers;
- GLM as the public math dependency;
- a static engine library;
- a minimal sandbox application.

The generated [API reference](https://vinaykomaravolu.github.io/VShade/) is
published with GitHub Pages. Documentation source and local build instructions
are in [docs/api.md](docs/api.md).

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

The sandbox opens a dark window. Press Escape or use the window close button
to exit.

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
    void OnUpdate(float deltaTime) override {
        // Update the active scene here.
    }
};

SHADE_ENGINE_MAIN(MyGame)
```

Input is stored once per frame, so it does not need a `Window` argument:

```cpp
if (vshade::platform::Input::is_key_pressed(vshade::platform::KeyCode::Escape)) {
    Close();
}

const glm::vec2 movement = vshade::platform::Input::mouse_delta();
```

Use `is_key_down` for a held key, `is_key_pressed` for its first frame down,
and `is_key_released` for its first frame up. Mouse buttons provide the same
three states.

Other core systems have focused headers:

```cpp
#include <core/assert.hpp>
#include <core/entrypoint.hpp>
#include <core/filesystem.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <platform/input.hpp>
#include <platform/keycode.hpp>
#include <platform/mousecode.hpp>
#include <platform/window.hpp>
```

Modern OpenGL function loading is intentionally not part of this first step.
Add a loader such as GLAD before implementing shaders, vertex buffers, or other
OpenGL functions newer than the platform's base OpenGL interface.
