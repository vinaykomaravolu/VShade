# Scripting

This example keeps user-authored native scripts in separate headers, like game
code normally would:

```text
22_scripting/
|-- main.cpp
`-- assets/
    |-- SpinScript.hpp
    `-- BounceScript.hpp
```

The left entity spins, the center entity bounces, and the right entity has
both scripts. This demonstrates that `ScriptComponent` stores an ordered list
and that one entity can run multiple behaviors.

## Registering a C++ script

```cpp
types().script<SpinScript>("Example.Spin");
```

This registers a factory with the application's native-script registry:

- `SpinScript` is the compiled C++ class the factory constructs.
- `"Example.Spin"` is its stable runtime identifier.
- `SpinScript` must derive from `NativeScript` and be default-constructible.

The string is what an entity stores—not a C++ pointer or compiler-specific
type name:

```cpp
entity.add<ScriptComponent>(ScriptComponent{{
    {.typeName = "Example.Spin"},
}});
```

When `playScene(scene)` starts, `NativeScriptSystem` reads that name, asks the
registry to construct `SpinScript`, attaches the owning entity and runtime
context, and invokes `onCreate()`. It then calls `onUpdate()` and
`onFixedUpdate()` while enabled, followed by `onDestroy()` when removed or when
the scene stops.

The stable name also survives scene serialization. Renaming the C++ class does
not break saved scenes as long as it is still registered as `"Example.Spin"`.
Changing that string requires migrating existing scene data.

Next: `23_simple_2d_game`.
