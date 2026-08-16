# VShade examples

These applications form a progressive tutorial for VShade. Each directory is
a standalone executable with its own source and teaching README. The numbered
examples begin with the application loop, introduce low-level rendering, and
then move to the component-driven scene runtime.

```sh
cmake -S . -B build-examples -DVSHADE_BUILD_EXAMPLES=ON -DBUILD_TESTING=OFF
cmake --build build-examples --config Debug
```

## Building from Cursor

With the CMake Tools extension installed, open the command palette and run
`CMake: Select Configure Preset`, then choose
`Examples (Visual Studio Debug)`. Run `CMake: Configure`, followed by
`CMake: Set Build Target`. Targets such as
`VShadeExample_05_camera_2d` will then appear in Cursor's target list.

The examples do not appear under the normal configuration because
`VSHADE_BUILD_EXAMPLES` deliberately defaults to `OFF`.

Executables are written to `build-examples/examples/bin`. Every example that
uses an external file keeps its own copy under that example's `assets/`
directory. `ExampleSupport::asset()` resolves those source-local assets, so an
example never depends on files under `tests/` or `sandbox/`.

`features/` contains focused references. A README-only feature directory means
the feature is deliberately planned but not yet supported by the engine.

## Learning path

| # | Example | Main idea |
|---:|---|---|
| 01 | `hello_window` | Application lifecycle, window, and clear color |
| 02 | `triangle` | Mesh data, shaders, materials, and a draw call |
| 03 | `input` | Keyboard, mouse, cursor delta, and scrolling |
| 04 | `textured_quad` | Loading and drawing a texture |
| 05 | `camera_2d` | Orthographic movement and zoom |
| 06 | `renderer_2d` | High-level batched quads |
| 07 | `scene_entities` | Entities and built-in or custom components |
| 08 | `sprite_scene` | ECS sprites rendered by `SceneRuntime` |
| 09 | `scene_serialization` | Saving and loading scenes with UUIDs |
| 10 | `framebuffer` | Offscreen rendering and pixel readback |
| 11 | `cube` | Perspective rendering and depth |
| 12 | `camera_3d` | Free-fly camera controls |
| 13 | `lighting` | Environment, directional, and point lights |
| 14 | `loading_model` | Loading a GLB through the asset manager |
| 15 | `materials` | Albedo, metallic, and roughness |
| 16 | `model_hierarchy` | GLB nodes and mesh primitives |
| 17 | `physics_2d` | A dynamic body falling onto a floor |
| 18 | `physics_collisions` | Callbacks, sensors, and filtering |
| 19 | `audio` | Music/SFX groups and audio voices |
| 20 | `audio_3d` | Listener state and spatial attenuation |
| 21 | `asset_manager` | Typed loading, caching, and handles |
| 22 | `scripting` | Registering and attaching a native script |
| 23 | `simple_2d_game` | A tiny integrated 2D game |
| 24 | `simple_3d_game` | Models, lights, physics, audio, and input |
| 25 | `rendering_workflows` | Runtime, scoped, and manual rendering paths |
| 26 | `direct_3d_physics` | Direct renderer, physics world, models, audio, and a user header |

## Feature references

The runnable feature demos are `multiple_textures`, `transparency`,
`multiple_lights`, `normal_mapping`, and `render_to_texture`. `instancing`,
`shadow_mapping`, `post_processing`, and `mouse_picking` have short roadmap
READMEs instead of misleading pseudo-examples; they should become executables
when those engine APIs exist.
