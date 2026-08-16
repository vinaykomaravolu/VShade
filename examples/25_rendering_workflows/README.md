# Rendering Workflows

This example renders the same rotating model through VShade's three rendering
workflows. Press **1**, **2**, or **3** to switch; the background changes so
the active path is obvious. Press Escape to exit.

## 1: Play an ECS scene

`playScene(m_scene)` starts `SceneRuntime`. The application automatically
consumes camera, model, environment, and light components before `onRender()`.
Use this for ordinary levels and gameplay.

## 2: Scoped direct rendering

`Renderer3D::scopedScene(camera)` returns an RAII submission scope. Draw calls
are explicit, while the scope guarantees `endScene()` even after an early
return. Use this for custom render systems, tools, and focused rendering code.

## 3: Manual direct rendering

`Renderer3D::beginScene(camera)`, `drawModel(...)`, and `endScene()` expose the
same direct renderer without RAII protection. This is useful for lower-level
engine code where the begin/end lifetime must be controlled explicitly.

All three paths use `assets/DamagedHelmet.glb`; only ownership and lifecycle
control differ.
