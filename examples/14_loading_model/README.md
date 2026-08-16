# Loading a 3D Model

## What you'll learn

- Loading a GLB through `AssetManager::loadResource<Model>`
- Creating a perspective camera
- Submitting every primitive in a model hierarchy

The asset manager caches the model. `Renderer3DSceneScope::draw` traverses its
nodes and submits their meshes/materials. Next: `15_materials`.
