# VShade

VShade is a small C++20 OpenGL engine runtime scaffold. It currently provides:

- a cross-platform GLFW window and OpenGL 3.3 context;
- engine and application logging through spdlog;
- GLM as the public math dependency;
- a static engine library;
- a minimal sandbox application.

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

## Use the library from another CMake target

When the engine and game are in the same source tree:

```cmake
add_subdirectory(path/to/VShade)

add_executable(MyGame main.cpp)
target_link_libraries(MyGame PRIVATE VShade::Engine)
```

Then include the public API with:

```cpp
#include <VShade/VShade.hpp>
```

Modern OpenGL function loading is intentionally not part of this first step.
Add a loader such as GLAD before implementing shaders, vertex buffers, or other
OpenGL functions newer than the platform's base OpenGL interface.
