# Direct 3D Physics

This deliberately low-level example does **not** use `playScene()`,
`SceneRuntime` play mode, `Renderer3D::scopedScene()`, or the shared example
helper. It shows the normal/manual APIs in one small application.

The example:

- loads `DamagedHelmet.glb` and `round_platform.glb` through `AssetManager`;
- loads `sample-3.wav` as an `AudioClip`;
- loads `shaders/helmet.vs` and `helmet.fs` as a custom shader pair;
- includes the game-owned `assets/HelmetBody.hpp` class;
- includes the user-authored `assets/HelmetResetScript.hpp` native script;
- creates and steps a `PhysicsWorld3D` directly;
- synchronizes the dynamic physics pose into a render transform;
- plays the loaded sound when the helmet first hits the platform; and
- renders with `beginScene()`, `drawModel()`, and `endScene()`.

The helmet shader follows Renderer3D's required `model`, `view`, and
`projection` contract. It samples the imported helmet albedo, applies the
renderer-provided ambient/directional light, and adds a pulsing blue edge glow
through custom `accentColor` and per-draw `time` uniforms.

Because native scripts require an owning entity and a `ScriptContext`, the
example creates a tiny scene only as a script host. It directly registers
`HelmetResetScript`, constructs `NativeScriptSystem`, attaches the host scene,
forwards variable/fixed updates, and detaches it during shutdown. The script
handles R and invokes a game-owned reset action. `playScene()` would normally
perform that lifecycle plumbing automatically.

## Controls

- **R**: drop the helmet again
- **Escape**: exit

This is useful when building custom engine systems. For normal level code,
the component-driven `playScene()` workflow removes most of this plumbing.
