# Golden-image renderer testing

VShade uses Catch2 golden-image tests to verify rendered output without showing
a window. A visual test creates a hidden GLFW window and OpenGL context,
renders a deterministic scene into an offscreen framebuffer, reads the pixels
back to CPU memory, and compares them with a committed PNG reference.

## Locations

```text
tests/
├── golden/
│   └── colored_triangle.png       # committed reference image
├── visual/
│   ├── imagecomparison.hpp
│   └── imagecomparison.cpp
└── renderer_visual_tests.cpp      # Catch2 visual test cases

build/tests/visual-output/          # generated failure artifacts
```

Committed golden images and generated test output are deliberately separate.
Nothing in the test code writes to `tests/golden`.

## Running the tests

Visual tests are ordinary Catch2 cases registered through CTest:

```sh
cmake -S . -B build
cmake --build build --config Debug --target VShadeTests
ctest --test-dir build -C Debug --output-on-failure -R "Offscreen"
```

The window uses `WindowConfig::visible = false`, so it is never displayed. The
current scene renders at 512 by 512 pixels into `renderer::Framebuffer`, not
the default window framebuffer.

Linux CI runs CTest through Xvfb because GLFW still requires a window-system
connection even when the native window is hidden.

## Comparison behavior

The comparison is not byte-for-byte. Each test supplies a
`ComparisonTolerance` with:

- maximum per-channel error before a pixel is counted as different;
- maximum mean absolute channel error across the complete image;
- maximum error allowed for any individual channel;
- maximum percentage of pixels allowed outside the per-channel tolerance.

`ComparisonResult` reports:

- whether image dimensions match;
- mean pixel error;
- maximum pixel error;
- number of pixels outside tolerance;
- percentage of pixels outside tolerance;
- overall pass or failure.

The Catch2 failure message prints these statistics and the artifact directory.

## Failure artifacts

When a golden comparison fails, the following files are written beneath
`build/tests/visual-output/<test-name>`:

- `actual.png` — the newly rendered image;
- `expected.png` — a copy of the committed reference;
- `difference.png` — amplified RGB differences for visual inspection.

The difference image multiplies channel differences to make small variations
easier to see. Missing regions caused by different image dimensions are shown
in magenta.

GitHub Actions uploads the visual output directory as the
`visual-test-artifacts` artifact whenever the coverage test job fails.

## Updating a golden image

Golden images are never overwritten automatically. When a reference is
missing, the test writes only a candidate `actual.png` into the build output
and fails with instructions.

To accept an intentional renderer change:

1. Run the affected visual test.
2. Inspect `actual.png`, `expected.png`, and `difference.png`.
3. Confirm that the rendering change is expected on supported drivers.
4. Manually copy the reviewed `actual.png` into the matching `tests/golden`
   path.
5. Run the test again and commit the updated golden with the renderer change.

This explicit review step prevents test failures or driver differences from
silently becoming the new expected output.

## Adding another visual test

For each deterministic scene:

1. Create `HiddenRenderContext` before GPU resources.
2. Render into a small `Framebuffer` with a fixed resolution.
3. Disable state that introduces unnecessary differences, such as dithering.
4. Use fixed geometry, shaders, colors, matrices, and texture data.
5. Call `Framebuffer::readPixels()` to obtain top-to-bottom RGBA8 pixels.
6. Compare through `compareAgainstGolden()` with documented tolerances.
7. Give the test its own golden filename and output subdirectory.

Avoid time-dependent animation, random values, system fonts, compressed
textures, and undefined OpenGL behavior in golden scenes.
