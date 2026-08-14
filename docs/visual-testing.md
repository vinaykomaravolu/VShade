# Golden-image renderer testing

VShade uses Catch2 golden-image tests to verify real OpenGL output through a
hidden GLFW window. Each visual test renders a deterministic scene into a
`Framebuffer`, reads RGBA8 pixels back to CPU memory, and compares them with a
committed PNG.

## Layout

```text
tests/
|-- golden/
|   |-- colored_triangle.png
|   |-- material_unlit_quad.png
|   |-- renderer2d_scene.png
|   |-- renderer3d_lit_cube.png
|   |-- rotating_cube.png
|   `-- textured_quad.png
|-- visual/
|   |-- ImageComparison.cpp
|   |-- ImageComparison.hpp
|   `-- RenderFixture.hpp
`-- *VisualTests.cpp

build/tests/visual-output/   # generated only when comparison fails
```

Tests cover flat and indexed geometry, texture loading/sampling, model-view-
projection transforms, depth and cull behavior, 2D submission, unlit
materials, and directional lighting. Timing never determines a reference
frame.

## Run

```sh
cmake -S . -B build
cmake --build build --config Debug --target VShadeTests
ctest --test-dir build -C Debug --output-on-failure
```

Linux CI runs the tests through Xvfb because GLFW still needs a window-system
connection for an invisible native window.

## Comparison and artifacts

Comparison tolerates small cross-driver channel differences while enforcing
limits for mean error, maximum error, and the percentage of pixels outside
the channel tolerance. On failure, the test writes:

- `actual.png`: current renderer output
- `expected.png`: committed reference
- `difference.png`: amplified per-channel differences

GitHub Actions uploads the output directory when the Linux visual job fails.

## Updating a golden

Golden files are never overwritten automatically. Review `actual.png` and
`difference.png`, confirm that the renderer change is intentional, then
replace only the corresponding file under `tests/golden`. Run the complete
suite again before committing the new baseline. Never accept a golden merely
to make a failure disappear; first explain the rendering change that produced
it.
