# Game-facing API

VShade uses progressive disclosure. `<vshade/Game.hpp>` is the default surface
for game code; focused subsystem headers remain the advanced surface.

## Ownership

`Application` owns `EngineServices` for the process lifetime and one
`SceneRuntime` for play mode. Assets and global music survive scene changes.
Physics bodies, native script instances, scene audio voices, camera selection,
lighting collection, rendering, and debug overlays are scoped to the active
scene and are stopped automatically.

```text
Application
  EngineServices: assets, global audio, scripts/types
  SceneRuntime
    NativeScriptSystem
    PhysicsSystem2D / PhysicsSystem3D
    SceneAudioSystem
    SceneRenderer
```

## Programmatic scene

```cpp
auto model = assets().reference<vshade::renderer::Model>("models/ball.glb");
auto ball = scene.create("Ball");
ball.transform().setPosition({0.0F, 8.0F, 0.0F});
ball.add<vshade::scene::ModelRendererComponent>(model, true);
ball.add<vshade::scene::RigidBody3DComponent>().settings.type =
    vshade::physics::BodyType::Dynamic;
ball.add<vshade::scene::Collider3DComponent>(
    vshade::scene::Collider3DComponent{
        .shape = vshade::physics::SphereShape3D{0.5F},
    }
);
```

Components are declarations consumed by the runtime. `Entity::get`, `tryGet`,
`has`, `set`, and `remove` are concise aliases; the older component-named APIs
remain compatible.

## Scripts and physics

Native scripts receive a `ScriptContext` through `context()`. It provides the
owning entity and typed scene, asset, audio, runtime, and physics access. Use
`context().physics3D().applyImpulse(entity(), impulse)` or scene-mapped
`raycast()` without maintaining a separate UUID-to-body table.

Register project types once through `types().script<T>(stableName)` and
`types().component<T>(stableName)`. Bindings for unavailable future language
backends remain serialized and produce runtime diagnostics rather than being
silently discarded.

## Reusable and advanced content

`Scene::instantiate()` creates an independent play-mode copy with stable scene
identity. `Prefab` is a reusable scene template; `scene.instantiate(prefab)`
copies it with fresh entity UUIDs and remapped parent relationships.

Install custom systems with `SceneRuntime::addSystem()` at before/after update,
physics, or render phases. For deeper control, use `PhysicsWorld2D/3D`,
`Renderer2D/3D`, `AudioEngine`, asset loaders, and `Scene::view` directly.
