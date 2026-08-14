# Scenes and serialization

`vshade::scene::Scene` owns an EnTT registry. `Entity` is a non-owning handle
containing an EnTT runtime identifier and its owning scene. Every entity
created through `Scene` receives these persistent components:

- `UUIDComponent`: stable 64-bit identity used by files and references.
- `TagComponent`: human-readable entity name.
- `TransformComponent`: position, rotation, and scale.

`SpriteRendererComponent` is optional. It stores a texture asset path instead
of a `Texture2D` pointer, allowing the saved value to remain valid between
runs. An asset system can resolve that path to a GPU resource at runtime.

## Creating and duplicating entities

```cpp
vshade::scene::Scene scene("Example");
vshade::scene::Entity player = scene.createEntity("Player");

player.component<vshade::scene::TransformComponent>().transform.setPosition(
    {0.0F, 1.0F, 0.0F}
);
player.addComponent<vshade::scene::SpriteRendererComponent>(
    std::filesystem::path("assets/player.png")
);

vshade::scene::Entity copy = scene.duplicateEntity(player);
```

The duplicate receives a new UUID. Its tag, transform, and optional sprite
component are copied. EnTT's numeric handle is only a runtime implementation
detail and is never serialized.

## Saving and loading

```cpp
vshade::scene::SceneSerializer serializer(scene);

if (!serializer.serialize("assets/scenes/example.vscene")) {
    // Report the write failure.
}

if (!serializer.deserialize("assets/scenes/example.vscene")) {
    // The existing scene remains unchanged after a load failure.
}
```

Scene files use indented JSON:

```json
{
  "FormatVersion": 2,
  "Scene": "Example",
  "Entities": [
    {
      "Entity": "128374",
      "Tag": "Player",
      "Transform": {
        "Position": [0.0, 1.0, 0.0],
        "Rotation": [0.0, 0.0, 0.0, 1.0],
        "Scale": [1.0, 1.0, 1.0]
      },
      "SpriteRenderer": {
        "Texture": "assets/player.png",
        "Color": [1.0, 1.0, 1.0, 1.0],
        "Tiling": [1.0, 1.0],
        "SortingLayer": 0
      }
    }
  ]
}
```

UUIDs are decimal strings so tools that internally represent JSON numbers as
floating point do not lose 64-bit precision. Rotation is stored as an XYZW
quaternion so saving and loading does not introduce an Euler-angle conversion.

The built-in UUID, tag, transform, and sprite components are handled directly.
Game-defined components can opt into duplication and serialization through
`SceneComponentRegistry::registerComponent`.

```cpp
struct HealthComponent { int points = 100; };

using Registry = vshade::scene::SceneComponentRegistry;
Registry::registerComponent<HealthComponent>("Health");
```

reflect-cpp discovers the public aggregate fields, so no JSON conversion
callbacks or field lists are needed. For prototypes, the component name can
also be inferred from the unqualified C++ type:

```cpp
Registry::registerComponent<HealthComponent>(); // "HealthComponent"
```

Prefer an explicit stable name for shipped scene files so renaming or moving
the C++ type does not change the file format. Components containing runtime
resources or requiring migration logic can still provide custom callbacks:

```cpp
Registry::registerComponent<WeaponComponent>(
    "Weapon",
    [](const WeaponComponent& weapon) {
        return Registry::Json{{"Definition", weapon.definitionPath}};
    },
    [](const Registry::Json& json) {
        WeaponComponent weapon;
        weapon.definitionPath = json.at("Definition").get<std::string>();
        return weapon;
    }
);
```

Register each component once during application startup, before loading or
duplicating scenes. Unknown serialized component names fail loading with a
diagnostic instead of silently discarding data.

The writer emits format version 2. Version 1 files using XYZ Euler rotations
are still accepted and are upgraded to quaternion rotations when next saved.
