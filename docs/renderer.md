# Renderer architecture and usage

VShade provides an OpenGL 3.3 renderer with low-level drawing operations and
high-level 2D and 3D scene submission. OpenGL remains the only backend while
the runtime API is still evolving.

## Public renderer modules

```text
engine/include/renderer/
|-- buffer.hpp
|-- camera.hpp
|-- cameracontroller.hpp
|-- framebuffer.hpp
|-- material.hpp
|-- materialparameters.hpp
|-- mesh.hpp
|-- renderer.hpp
|-- renderer2d.hpp
|-- renderer3d.hpp
|-- rendertypes.hpp
|-- shader.hpp
|-- texture.hpp
`-- vertexarray.hpp
```

- `Renderer` owns initialization, pipeline state, viewport state, clears,
  drawing, device limits, and frame statistics.
- `Buffer`, `VertexArray`, `Shader`, `Texture2D`, and `Framebuffer` are
  move-only RAII wrappers around GPU resources.
- `Camera` stores validated view and projection matrices.
- `CameraController` supplies fly-camera WASD and mouse-look behavior.
- `Mesh` combines a vertex array with a primitive topology.
- `Material` retains an optional shader and texture plus albedo, basic
  lighting properties, and typed custom shader parameters.
- `Renderer2D` queues layer-sorted quads and sprites.
- `Renderer3D` queues meshes with materials and a directional light.

OpenGL headers, the generated GLAD loader, and enum conversions remain private
under `engine/src/renderer`.

## Lifetime

`Application` establishes this order:

```text
Create window and OpenGL context
  -> initialize Renderer
  -> onStart
  -> poll input and update time
  -> zero or more onFixedUpdate calls
  -> onUpdate
  -> begin renderer frame and onRender
  -> present
  -> onShutdown
  -> release renderer-owned defaults
  -> shut down Renderer
  -> destroy window/context
```

Create game-owned GPU resources in or after `onStart` and release them in
`onShutdown`. Resource operations require an initialized renderer and a live
context. `Renderer2D` and `Renderer3D` create their built-in shaders, white
textures, and static geometry lazily, reuse them between frames, and release
them during `Renderer::shutdown`.

## Pipeline state

Use `Renderer` for blend, cull, polygon, depth, dithering, and viewport state.
`Renderer::pipelineState()` and `Renderer::viewport()` expose the state tracked
by the facade. `Renderer::pushPipelineState()` returns a movable RAII guard:

```cpp
{
    auto stateGuard = vshade::renderer::Renderer::pushPipelineState();
    vshade::renderer::Renderer::setDepthTesting(false);
    vshade::renderer::Renderer::setBlending(true);
    // The captured state is restored when stateGuard leaves scope.
}
```

High-level 2D and 3D scenes retain one of these guards from `beginScene` until
`endScene`. The previous state is therefore restored after normal submission,
exceptions, or renderer shutdown without duplicating manual cleanup paths.

Direct OpenGL state changes bypass this tracking and should be avoided in game
code.

## Framebuffers

`Framebuffer` owns an RGBA8 color attachment and a depth/stencil attachment.
`bind()` saves the current draw/read targets and viewport, then selects the
offscreen target. `Framebuffer::unbind()` must be paired with `bind()` and
restores the saved targets and viewport. Nested binds are supported.

```cpp
framebuffer.bind();
vshade::renderer::Renderer::clear();
// Render the offscreen pass.
vshade::renderer::Framebuffer::unbind();
// The previous target and viewport are active again.
```

Do not resize a framebuffer while it is bound. `readPixels()` returns tightly
packed, top-to-bottom RGBA8 bytes and restores its temporary readback state.

## Camera and controller

`Camera` is rendering data: it owns a view matrix and projection matrix. The
controller is optional behavior that changes a camera's view.

```cpp
vshade::renderer::Camera camera;
camera.setPerspective(glm::radians(45.0F), aspect, 0.1F, 100.0F);
camera.lookAt({3.0F, 2.0F, 4.0F}, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});

vshade::renderer::CameraController controller(camera);
controller.update(deltaTime); // Reads the engine Input service.
```

The controller can also use an explicit `CameraControllerInput`, which is
useful for remapping controls, replay, and unit tests. If another system moves
the camera directly, call `syncFromCamera()` before the controller updates it
again. A controller is non-copyable and must not outlive its camera.

The sandbox captures the cursor while the right mouse button is held so mouse
look is not limited by a screen edge. Focus loss clears held input and releases
the cursor.

## Renderer3D shader contract

Custom material shaders use these vertex locations:

| Location | Input |
|---:|---|
| 0 | position (`vec3`) |
| 1 | normal (`vec3`) |
| 2 | texture coordinate (`vec2`) |

They must expose active `mat4` uniforms named `model`, `view`, and
`projection`. `Renderer3D` rejects overrides missing those uniforms. The
The built-in material uniforms are an implementation detail rather than part
of `Renderer3DShaderInterface`.

The expected transform is deliberately conventional:

```glsl
gl_Position = projection * view * model * vec4(aPos, 1.0);
```

The built-in lit shader is a small directional-light model, not PBR.
Roughness controls diffuse scattering and metallic reduces diffuse response.

### Custom shader parameters

Store values shared by every use of a material on the material itself:

```cpp
material.parameters().set("effectColor", vshade::math::Vec3{0.2F, 0.5F, 1.0F});
material.parameters().set("effectStrength", 0.75F);
```

For one object, pass `DrawParameters` to `drawMesh`. Draw values override
material values with the same name:

```cpp
vshade::renderer::DrawParameters drawParameters;
drawParameters.set("effectStrength", 1.0F);

vshade::renderer::Renderer3D::drawMesh(
    transform,
    mesh,
    material,
    drawParameters
);
```

Supported values are `int`, `float`, `Vec2`, `Vec3`, `Vec4`, `Mat4`, and a
shared `Texture2D`. The renderer copies material and draw parameters into its
queue, so changing either collection after `drawMesh` does not alter an
already-submitted object. Shader uniform locations are cached by `Shader`.
Inactive custom uniforms are ignored. `model`, `view`, and `projection` stay
renderer-owned and cannot be replaced by custom parameters.

## Sandbox

The sandbox generates a 24-vertex indexed cube and submits it twice with a
shared checkerboard texture, shader, and lit material. The left cube uses
material-level custom uniforms; the right cube supplies animated per-draw
overrides. Its render scope also demonstrates `PipelineStateGuard` and
`PipelineState` by providing a wireframe toggle.

Controls:

- `W`, `A`, `S`, `D`: fly the camera
- Hold right mouse: capture the cursor and look around
- Mouse wheel: change movement speed
- `P`: toggle fill/wireframe pipeline mode
- Escape: close the application

## Testing

Catch2 tests exercise renderer validation and actual OpenGL behavior through a
hidden GLFW context. Golden-image tests cover a colored triangle, textured
quad, rotating cube, `Renderer2D`, unlit material, and directional lighting.
State tests also cover persistent high-level resources, pipeline restoration,
framebuffer viewport restoration, shader contracts, and camera movement.

See [Visual testing](visual-testing.md) for golden-image maintenance.

## Current scope

Implemented now:

- OpenGL 3.3 rendering and explicit pipeline state
- Buffers, vertex arrays, shaders, textures, and offscreen framebuffers
- Perspective/orthographic cameras and fly-camera controls
- Meshes, materials, 2D quads/sprites, and 3D mesh submission
- One basic directional light
- Frame statistics and visual regression tests

Not implemented yet:

- General asset management or model importing
- Scene/entity storage and serialization
- Sprite batching and advanced render-queue optimization
- Multiple lights, shadows, PBR, animation, or post-processing
- Audio, physics, editor tooling, or additional graphics backends
