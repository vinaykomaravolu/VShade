# Renderer foundation

VShade now has a compact OpenGL 3.3 renderer foundation for both 2D and 3D
games. The renderer is intentionally OpenGL-only for now so the engine can
focus on useful rendering features before introducing multiple graphics
backends.

## What changed

The engine now:

- loads modern OpenGL functions with GLAD after GLFW creates a context;
- initializes and shuts down the renderer as part of `Application`;
- calls `Application::onRender()` once per frame;
- updates the OpenGL viewport when the framebuffer is resized;
- provides GPU buffers, vertex arrays, shaders, textures, cameras, and meshes;
- supports indexed triangle, line, and point drawing;
- records draw-call and index-count statistics for each frame.

GLAD 2.0.8 is pinned in `external/glad`. The generated OpenGL 3.3 core loader
is checked into `engine/src/renderer/opengl/generated`, so normal builds do not
need Python or GLAD's generator packages.

## Folder structure

```text
engine/
├── include/renderer/
│   ├── buffer.hpp
│   ├── camera.hpp
│   ├── framebuffer.hpp
│   ├── mesh.hpp
│   ├── renderer.hpp
│   ├── rendertypes.hpp
│   ├── shader.hpp
│   ├── texture.hpp
│   └── vertexarray.hpp
│
└── src/renderer/
    ├── buffer.cpp
    ├── camera.cpp
    ├── framebuffer.cpp
    ├── mesh.cpp
    ├── renderer.cpp
    ├── rendertypes.cpp
    ├── shader.cpp
    ├── texture.cpp
    ├── vertexarray.cpp
    └── opengl/
        ├── openglutils.hpp
        ├── openglutils.cpp
        └── generated/
```

Public game-facing headers live in `include/renderer`. OpenGL headers,
generated loader code, and backend conversion helpers remain private in
`src/renderer`.

## File responsibilities

### `renderer.hpp`

`Renderer` owns renderer-global operations:

- OpenGL function loading and default state;
- regional viewport and selective framebuffer clearing;
- blending, face-culling, polygon, and depth state;
- indexed and non-indexed draw calls;
- per-frame rendering statistics.

`Application` calls `Renderer::initialize()`, `Renderer::beginFrame()`, and
`Renderer::shutdown()` automatically. Game code normally uses the state and
drawing functions from `onRender()`.

### `rendertypes.hpp`

Contains engine-facing enums for:

- shader vertex data types;
- static and dynamic buffer usage;
- triangle, line, and point topology;
- framebuffer clear flags;
- blend factors, cull faces, winding, and polygon modes;
- depth comparison functions;
- texture formats, filters, and wrapping.

The renderer implementation converts these enums to OpenGL values privately.

### `buffer.hpp`

Provides:

- `BufferElement`, describing one vertex attribute;
- `BufferLayout`, calculating attribute offsets and vertex stride;
- `VertexBuffer`, storing vertex data on the GPU;
- `IndexBuffer`, storing 32-bit indices on the GPU.

Example layout:

```cpp
vshade::renderer::BufferLayout layout{
    {"position", vshade::renderer::ShaderDataType::Float3},
    {"color", vshade::renderer::ShaderDataType::Float4},
};
```

### `vertexarray.hpp`

`VertexArray` connects vertex buffers, their layouts, and an index buffer. It
configures OpenGL vertex attributes when a vertex buffer is added and retains
shared ownership of the attached buffers.

### `shader.hpp`

`Shader` compiles and links one GLSL vertex shader and fragment shader. It can
use source strings or load two text files with `Shader::fromFiles()`.

It currently supports uniforms for:

- integers and floats;
- `Vec2`, `Vec3`, and `Vec4`;
- `Mat4`.

Compilation and linking failures throw `std::runtime_error` with the OpenGL
diagnostic log.

### `texture.hpp`

`Texture2D` creates a raw 2D OpenGL texture and optionally uploads tightly
packed pixel data. It supports one-, three-, and four-channel 8-bit formats,
nearest or linear filtering, and repeat or clamp-to-edge wrapping.

Image-file decoding is not included yet. A future asset layer can use an image
library to decode PNG or JPEG data before passing pixels to `Texture2D`.

### `camera.hpp`

`Camera` stores view and projection matrices. It supports:

- explicit matrices;
- right-handed look-at views;
- perspective projection for 3D;
- orthographic projection for 2D or 3D.

`viewProjection()` returns `projection * view`, ready to upload to a shader.

### `framebuffer.hpp`

`Framebuffer` owns an offscreen RGBA8 color attachment and a depth-stencil
attachment. Binding it updates the viewport to its dimensions. `readPixels()`
returns top-to-bottom RGBA8 CPU data suitable for image encoding and visual
regression testing.

### `mesh.hpp`

`Mesh` is a small renderable geometry object. It retains a `VertexArray` and
the primitive topology used to render that geometry.

## Frame lifecycle

The application lifecycle is now:

```text
Create GLFW window and OpenGL context
    ↓
Initialize Renderer and GLAD
    ↓
Call onStart()
    ↓
Poll events and update frame time
    ↓
Call onUpdate()
    ↓
Reset renderer statistics
    ↓
Call onRender()
    ↓
Swap window buffers
    ↓
Call onShutdown()
    ↓
Destroy game-owned renderer resources
    ↓
Shut down Renderer and destroy the window
```

Renderer resources should be created in or after `onStart()`, because the
OpenGL context and renderer are available at that point. The base
`Application` keeps the context alive while members of the derived game
application are destroyed.

## Basic rendering usage

The sandbox contains a complete colored-triangle example. It creates its
buffers, vertex layout, vertex array, and file-backed shader in `onStart()`.
Each frame binds that shader and submits the indexed triangle:

```cpp
void onRender() override {
    vshade::renderer::Renderer::setClearColor({0.05F, 0.06F, 0.09F, 1.0F});
    vshade::renderer::Renderer::clear();

    m_shader->bind();
    vshade::renderer::Renderer::drawIndexed(*m_triangle);
}
```

The GLSL sources live in `sandbox/shaders`. CMake copies them beside the
sandbox executable, and `Shader::fromFiles()` loads them at startup.

Viewport origins are explicit so the renderer can target an editor panel,
split-screen region, or part of a larger render target:

```cpp
vshade::renderer::Renderer::setViewport(x, y, width, height);
```

Depth testing and depth writes are controlled separately. A transparent pass
can test transparent fragments against opaque geometry without changing the
depth buffer:

```cpp
vshade::renderer::Renderer::setDepthTesting(true);
vshade::renderer::Renderer::setDepthWrite(false);
vshade::renderer::Renderer::setBlending(true);

// Draw transparent geometry back-to-front.

vshade::renderer::Renderer::setDepthWrite(true);
```

A complete indexed draw will follow this order:

1. Create a `VertexBuffer` and assign its `BufferLayout`.
2. Create an `IndexBuffer`.
3. Attach both buffers to a `VertexArray`.
4. Create and bind a `Shader`.
5. Upload camera and model matrices.
6. Call `Renderer::drawIndexed()`, call `Renderer::drawArrays()` for
   non-indexed vertices, or create a `Mesh` and call `Renderer::draw()`.

## Current scope

This foundation does not yet include:

- image-file loading;
- materials;
- general-purpose framebuffer attachment configurations and render-to-texture workflows;
- sprite batching or `Renderer2D`;
- model-file loading;
- lighting, shadows, or post-processing;
- multiple graphics APIs.

Recommended next milestones are a colored triangle, a textured quad, a basic
3D mesh with a camera, and then a batched 2D renderer.

## Testing

Renderer tests currently verify logic that does not require a visible OpenGL
window:

- shader data-type sizes and component counts;
- vertex layout offsets and stride;
- camera view-projection behavior;
- invalid mesh construction;
- renderer state before context initialization.

GPU integration is exercised by compiling the sandbox against the complete
renderer and GLAD loader. Catch2 visual tests also create a hidden GLFW context,
render into an offscreen framebuffer, and compare CPU-readback pixels with
committed golden PNGs. See [Golden-image renderer testing](visual-testing.md).
