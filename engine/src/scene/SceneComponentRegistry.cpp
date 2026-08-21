#include "scene/SceneComponentRegistry.hpp"

#include "scene/Components.hpp"

#include <algorithm>
#include <stdexcept>

namespace vshade::scene {

std::vector<SceneComponentRegistry::Handler>& SceneComponentRegistry::mutableHandlers() {
    static std::vector<SceneComponentRegistry::Handler> handlers;
    return handlers;
}

void SceneComponentRegistry::add(Handler handler) {
    if (handler.name.empty()) {
        throw std::invalid_argument("A serialized component name cannot be empty");
    }
    if (handler.name == "UUID" || handler.name == "Tag" ||
        handler.name == "Transform" || handler.name == "SpriteRenderer" ||
        handler.name == "Camera" || handler.name == "ModelRenderer" ||
        handler.name == "AudioSource" || handler.name == "AudioListener" ||
        handler.name == "Light" || handler.name == "RigidBody2D" ||
        handler.name == "Collider2D" || handler.name == "RigidBody3D" ||
        handler.name == "Collider3D" || handler.name == "BoxCollider3D" ||
        handler.name == "SphereCollider3D" || handler.name == "CapsuleCollider3D" ||
        handler.name == "CylinderCollider3D" || handler.name == "MeshCollider3D" ||
        handler.name == "ConvexCollider3D" || handler.name == "Scripts" ||
        handler.name == "Parent" || handler.name == "HierarchyOrder" ||
        handler.name == "HierarchyState" ||
        handler.type == entt::type_hash<CameraComponent>::value() ||
        handler.type == entt::type_hash<ModelRendererComponent>::value() ||
        handler.type == entt::type_hash<RigidBody2DComponent>::value() ||
        handler.type == entt::type_hash<Collider2DComponent>::value() ||
        handler.type == entt::type_hash<RigidBody3DComponent>::value() ||
        handler.type == entt::type_hash<Collider3DComponent>::value() ||
        handler.type == entt::type_hash<BoxCollider3DComponent>::value() ||
        handler.type == entt::type_hash<SphereCollider3DComponent>::value() ||
        handler.type == entt::type_hash<CapsuleCollider3DComponent>::value() ||
        handler.type == entt::type_hash<CylinderCollider3DComponent>::value() ||
        handler.type == entt::type_hash<MeshCollider3DComponent>::value() ||
        handler.type == entt::type_hash<ConvexCollider3DComponent>::value() ||
        handler.type == entt::type_hash<ScriptComponent>::value() ||
        handler.type == entt::type_hash<ParentComponent>::value() ||
        handler.type == entt::type_hash<HierarchyOrderComponent>::value() ||
        handler.type == entt::type_hash<HierarchyStateComponent>::value()) {
        throw std::invalid_argument(
            "A custom component cannot reuse a built-in component type or name"
        );
    }

    auto& existing = mutableHandlers();
    if (std::ranges::any_of(existing, [&handler](const Handler& candidate) {
            return candidate.type == handler.type || candidate.name == handler.name;
        })) {
        throw std::invalid_argument("Component type or serialized name is already registered");
    }
    existing.push_back(std::move(handler));
}

const std::vector<SceneComponentRegistry::Handler>& SceneComponentRegistry::handlers() {
    return mutableHandlers();
}

} // namespace vshade::scene
