#include "scene/Prefab.hpp"

#include "scene/Scene.hpp"
#include "scene/SceneComponentRegistry.hpp"
#include "scene/SceneSerializer.hpp"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vshade::scene {

Prefab::Prefab(std::shared_ptr<const Scene> scene)
    : m_scene(std::move(scene)) {
    if (!m_scene) {
        throw std::invalid_argument("A prefab scene cannot be null");
    }
}

Prefab Prefab::fromEntity(const Scene& scene, const Entity root) {
    if (!scene.valid(root)) {
        throw std::invalid_argument("Cannot extract a prefab from an invalid entity");
    }

    const std::uint64_t rootUuid = root.uuid();
    std::vector<entt::entity> sources;
    std::unordered_set<std::uint64_t> included;
    std::vector<entt::entity> pending{root.handle()};
    while (!pending.empty()) {
        const entt::entity handle = pending.back();
        pending.pop_back();
        const std::uint64_t uuid = scene.m_registry.get<UUIDComponent>(handle).uuid;
        if (!included.insert(uuid).second) {
            continue;
        }
        sources.push_back(handle);
        for (const auto [childHandle, childUuid, relationship] :
             scene.m_registry.view<const UUIDComponent, const ParentComponent>().each()) {
            (void)childUuid;
            if (relationship.parentUuid == uuid) {
                pending.push_back(childHandle);
            }
        }
    }

    auto prefabScene = std::make_shared<Scene>(
        scene.m_registry.get<TagComponent>(root.handle()).tag
    );
    std::unordered_map<std::uint64_t, std::uint64_t> instanceToPrefabUuid;
    if (const auto* instance =
            scene.m_registry.try_get<PrefabInstanceComponent>(root.handle())) {
        for (const PrefabEntityLink& link : instance->entities) {
            instanceToPrefabUuid[link.instanceUuid] = link.prefabUuid;
        }
    }

    std::unordered_map<std::uint64_t, Entity> entities;
    for (const entt::entity handle : sources) {
        const auto& uuid = scene.m_registry.get<UUIDComponent>(handle);
        const auto& tag = scene.m_registry.get<TagComponent>(handle);
        std::uint64_t prefabUuid = uuid.uuid;
        if (!instanceToPrefabUuid.empty()) {
            if (const auto found = instanceToPrefabUuid.find(uuid.uuid);
                found != instanceToPrefabUuid.end()) {
                prefabUuid = found->second;
            } else {
                prefabUuid = prefabScene->generateUuid();
            }
        }
        entities.emplace(uuid.uuid, prefabScene->createEntityWithUuid(tag.tag, prefabUuid));
    }

    for (const entt::entity handle : sources) {
        const std::uint64_t sourceUuid = scene.m_registry.get<UUIDComponent>(handle).uuid;
        Entity destination = entities.at(sourceUuid);
        destination.component<TransformComponent>() =
            scene.m_registry.get<TransformComponent>(handle);

        const auto copy = [&]<typename Component>() {
            if (scene.m_registry.all_of<Component>(handle)) {
                destination.addComponent<Component>(
                    scene.m_registry.get<Component>(handle)
                );
            }
        };
        copy.template operator()<SpriteRendererComponent>();
        copy.template operator()<CameraComponent>();
        copy.template operator()<ModelRendererComponent>();
        copy.template operator()<AudioSourceComponent>();
        copy.template operator()<AudioListenerComponent>();
        copy.template operator()<LightComponent>();
        copy.template operator()<RigidBody2DComponent>();
        copy.template operator()<Collider2DComponent>();
        copy.template operator()<RigidBody3DComponent>();
        copy.template operator()<Collider3DComponent>();
        copy.template operator()<BoxCollider3DComponent>();
        copy.template operator()<SphereCollider3DComponent>();
        copy.template operator()<CapsuleCollider3DComponent>();
        copy.template operator()<CylinderCollider3DComponent>();
        copy.template operator()<MeshCollider3DComponent>();
        copy.template operator()<ConvexCollider3DComponent>();
        copy.template operator()<ScriptComponent>();
        copy.template operator()<HierarchyOrderComponent>();
        copy.template operator()<HierarchyStateComponent>();

        if (sourceUuid != rootUuid) {
            if (const auto* relationship =
                    scene.m_registry.try_get<ParentComponent>(handle);
                relationship != nullptr
                    && included.contains(relationship->parentUuid)) {
                destination.addComponent<ParentComponent>(ParentComponent{
                    entities.at(relationship->parentUuid).uuid()
                });
            }
        }
        for (const auto& handler : SceneComponentRegistry::handlers()) {
            handler.clone(
                scene.m_registry,
                handle,
                prefabScene->m_registry,
                destination.handle()
            );
        }
    }
    return Prefab{std::shared_ptr<const Scene>(std::move(prefabScene))};
}

core::Result<void> Prefab::save(const std::filesystem::path& path) const {
    SceneSerializer serializer(*m_scene);
    return serializer.serializeResult(path);
}

const Scene& Prefab::scene() const noexcept {
    return *m_scene;
}

} // namespace vshade::scene
