#include "scene/Scene.hpp"
#include "scene/SceneComponentRegistry.hpp"
#include "scene/Prefab.hpp"

#include "asset/AssetManager.hpp"
#include "core/Log.hpp"

#include <atomic>
#include <algorithm>
#include <limits>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace vshade::scene {

namespace {

[[nodiscard]] std::uint64_t initialUuidValue() {
    std::random_device random;
    const std::uint64_t high = static_cast<std::uint64_t>(random()) << 32U;
    const std::uint64_t low = static_cast<std::uint64_t>(random());
    const std::uint64_t value = high | low;
    return value == 0 ? 1 : value;
}

std::atomic<std::uint64_t> nextUuid{initialUuidValue()};

} // namespace

Scene::Scene(std::string name)
    : m_name(std::move(name)) {}
Scene::~Scene() = default;

Entity Scene::createEntity(std::string name) {
    return createEntityWithUuid(std::move(name), generateUuid());
}

Entity Scene::createEntityWithUuid(std::string name, const std::uint64_t uuid) {
    if (uuid == 0 || m_entitiesByUuid.contains(uuid)) {
        throw std::invalid_argument("Entity UUID must be non-zero and unique within its scene");
    }
    const entt::entity handle = m_registry.create();
    try {
        m_registry.emplace<UUIDComponent>(handle, uuid);
        m_registry.emplace<TagComponent>(handle, std::move(name));
        m_registry.emplace<TransformComponent>(handle);
        m_entitiesByUuid.emplace(uuid, handle);
    } catch (...) {
        m_registry.destroy(handle);
        throw;
    }
    return Entity(handle, *this, m_generation);
}

Entity Scene::duplicateEntity(const Entity source) {
    if (!valid(source)) {
        throw std::invalid_argument("Cannot duplicate an invalid or foreign entity");
    }

    Entity duplicate = createEntity(source.component<TagComponent>().tag + " Copy");
    try {
        duplicate.component<TransformComponent>() = source.component<TransformComponent>();
        if (source.hasComponents<SpriteRendererComponent>()) {
            duplicate.addComponent<SpriteRendererComponent>(
                source.component<SpriteRendererComponent>()
            );
        }
        if (source.hasComponents<CameraComponent>()) {
            duplicate.addComponent<CameraComponent>(
                source.component<CameraComponent>()
            );
        }
        if (source.hasComponents<ModelRendererComponent>()) {
            duplicate.addComponent<ModelRendererComponent>(
                source.component<ModelRendererComponent>()
            );
        }
        if (source.hasComponents<AudioSourceComponent>()) {
            duplicate.addComponent<AudioSourceComponent>(
                source.component<AudioSourceComponent>()
            );
        }
        if (source.hasComponents<AudioListenerComponent>()) {
            duplicate.addComponent<AudioListenerComponent>(
                source.component<AudioListenerComponent>()
            );
        }
        if (source.hasComponents<LightComponent>()) {
            duplicate.addComponent<LightComponent>(
                source.component<LightComponent>()
            );
        }
        if (source.hasComponents<RigidBody2DComponent>()) {
            duplicate.addComponent<RigidBody2DComponent>(
                source.component<RigidBody2DComponent>()
            );
        }
        if (source.hasComponents<Collider2DComponent>()) {
            duplicate.addComponent<Collider2DComponent>(
                source.component<Collider2DComponent>()
            );
        }
        if (source.hasComponents<RigidBody3DComponent>()) {
            duplicate.addComponent<RigidBody3DComponent>(
                source.component<RigidBody3DComponent>()
            );
        }
        if (source.hasComponents<Collider3DComponent>()) {
            duplicate.addComponent<Collider3DComponent>(
                source.component<Collider3DComponent>()
            );
        }
        const auto duplicateCollider = [&]<typename Component>() {
            if (source.has<Component>()) {
                duplicate.add<Component>(source.get<Component>());
            }
        };
        duplicateCollider.template operator()<BoxCollider3DComponent>();
        duplicateCollider.template operator()<SphereCollider3DComponent>();
        duplicateCollider.template operator()<CapsuleCollider3DComponent>();
        duplicateCollider.template operator()<CylinderCollider3DComponent>();
        duplicateCollider.template operator()<MeshCollider3DComponent>();
        duplicateCollider.template operator()<ConvexCollider3DComponent>();
        if (source.hasComponents<ScriptComponent>()) {
            duplicate.addComponent<ScriptComponent>(
                source.component<ScriptComponent>()
            );
        }
        if (source.hasComponents<ParentComponent>()) {
            duplicate.addComponent<ParentComponent>(
                source.component<ParentComponent>()
            );
        }
        for (const auto& handler : SceneComponentRegistry::handlers()) {
            handler.clone(m_registry, source.m_handle, m_registry, duplicate.m_handle);
        }
    } catch (...) {
        destroyEntity(duplicate);
        throw;
    }
    return duplicate;
}

std::unique_ptr<Scene> Scene::instantiate() const {
    auto instance = std::make_unique<Scene>(m_name);
    instance->m_environment = m_environment;

    const auto* entityStorage = m_registry.storage<entt::entity>();
    for (const auto [sourceHandle] : entityStorage->each()) {
        const auto& uuid = m_registry.get<UUIDComponent>(sourceHandle);
        const auto& tag = m_registry.get<TagComponent>(sourceHandle);
        Entity destination = instance->createEntityWithUuid(tag.tag, uuid.uuid);
        destination.component<TransformComponent>() =
            m_registry.get<TransformComponent>(sourceHandle);

        const auto copyComponent = [&]<typename Component>() {
            if (m_registry.all_of<Component>(sourceHandle)) {
                instance->m_registry.emplace<Component>(
                    destination.m_handle,
                    m_registry.get<Component>(sourceHandle)
                );
            }
        };
        copyComponent.template operator()<SpriteRendererComponent>();
        copyComponent.template operator()<CameraComponent>();
        copyComponent.template operator()<ModelRendererComponent>();
        copyComponent.template operator()<AudioSourceComponent>();
        copyComponent.template operator()<AudioListenerComponent>();
        copyComponent.template operator()<LightComponent>();
        copyComponent.template operator()<RigidBody2DComponent>();
        copyComponent.template operator()<Collider2DComponent>();
        copyComponent.template operator()<RigidBody3DComponent>();
        copyComponent.template operator()<Collider3DComponent>();
        copyComponent.template operator()<BoxCollider3DComponent>();
        copyComponent.template operator()<SphereCollider3DComponent>();
        copyComponent.template operator()<CapsuleCollider3DComponent>();
        copyComponent.template operator()<CylinderCollider3DComponent>();
        copyComponent.template operator()<MeshCollider3DComponent>();
        copyComponent.template operator()<ConvexCollider3DComponent>();
        copyComponent.template operator()<ScriptComponent>();
        copyComponent.template operator()<ParentComponent>();
        copyComponent.template operator()<PrefabInstanceComponent>();

        for (const auto& handler : SceneComponentRegistry::handlers()) {
            handler.clone(
                m_registry,
                sourceHandle,
                instance->m_registry,
                destination.m_handle
            );
        }
    }
    return instance;
}

Entity Scene::instantiate(const Prefab& prefab) {
    return instantiate(prefab, {});
}

Entity Scene::instantiate(
    const Prefab& prefab,
    const asset::AssetReference<Prefab>& source
) {
    const Scene& sourceScene = prefab.scene();
    std::unordered_map<std::uint64_t, Entity> entities;
    std::vector<Entity> created;
    std::uint64_t primarySourceUuid = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t firstSourceUuid = std::numeric_limits<std::uint64_t>::max();

    try {
        for (const auto [sourceHandle] :
             sourceScene.m_registry.storage<entt::entity>()->each()) {
            const auto& uuid = sourceScene.m_registry.get<UUIDComponent>(sourceHandle);
            const auto& tag = sourceScene.m_registry.get<TagComponent>(sourceHandle);
            Entity destination = createEntity(tag.tag);
            firstSourceUuid = std::min(firstSourceUuid, uuid.uuid);
            if (!sourceScene.m_registry.all_of<ParentComponent>(sourceHandle)) {
                primarySourceUuid = std::min(primarySourceUuid, uuid.uuid);
            }
            entities.emplace(uuid.uuid, destination);
            created.push_back(destination);
        }

        for (const auto [sourceHandle] :
             sourceScene.m_registry.storage<entt::entity>()->each()) {
            const std::uint64_t sourceUuid =
                sourceScene.m_registry.get<UUIDComponent>(sourceHandle).uuid;
            Entity destination = entities.at(sourceUuid);
            copyPrefabComponents(destination, sourceScene, sourceHandle, false);
            if (const auto* relationship =
                sourceScene.m_registry.try_get<ParentComponent>(sourceHandle)) {
                destination.addComponent<ParentComponent>(ParentComponent{
                    entities.at(relationship->parentUuid).uuid()
                });
            }
        }

        if (primarySourceUuid == std::numeric_limits<std::uint64_t>::max()) {
            primarySourceUuid = firstSourceUuid;
        }
        Entity root = primarySourceUuid == std::numeric_limits<std::uint64_t>::max()
            ? Entity{}
            : entities.at(primarySourceUuid);
        if (root && source.valid()) {
            PrefabInstanceComponent component{.prefab = source};
            component.entities.reserve(entities.size());
            for (const auto& [prefabUuid, entity] : entities) {
                component.entities.push_back({prefabUuid, entity.uuid()});
            }
            std::ranges::sort(
                component.entities,
                [](const PrefabEntityLink& left, const PrefabEntityLink& right) {
                    return left.prefabUuid < right.prefabUuid;
                }
            );
            root.add<PrefabInstanceComponent>(std::move(component));
        }
        return root;
    } catch (...) {
        for (auto iterator = created.rbegin(); iterator != created.rend(); ++iterator) {
            if (valid(*iterator)) destroyEntity(*iterator);
        }
        throw;
    }
}

void Scene::bindPrefabInstance(
    Entity root,
    asset::AssetReference<Prefab> source
) {
    if (!valid(root) || !source.valid()) {
        throw std::invalid_argument("Prefab instance root and source asset must be valid");
    }

    PrefabInstanceComponent component{.prefab = std::move(source)};
    std::vector<Entity> pending{root};
    std::unordered_set<std::uint64_t> visited;
    while (!pending.empty()) {
        const Entity entity = pending.back();
        pending.pop_back();
        if (!valid(entity) || !visited.insert(entity.uuid()).second) {
            continue;
        }
        component.entities.push_back({entity.uuid(), entity.uuid()});
        for (const Entity child : children(entity)) {
            pending.push_back(child);
        }
    }
    std::ranges::sort(
        component.entities,
        [](const PrefabEntityLink& left, const PrefabEntityLink& right) {
            return left.prefabUuid < right.prefabUuid;
        }
    );
    root.set<PrefabInstanceComponent>(std::move(component));
}

void Scene::applyPrefab(Entity instanceRoot, const Prefab& prefab) {
    if (!valid(instanceRoot) || !instanceRoot.has<PrefabInstanceComponent>()) {
        throw std::invalid_argument("applyPrefab requires a linked prefab instance root");
    }

    auto& instance = instanceRoot.get<PrefabInstanceComponent>();
    const math::Transform rootTransform = instanceRoot.transform();
    const Entity rootParent = parent(instanceRoot);

    std::unordered_map<std::uint64_t, std::uint64_t> prefabToInstance;
    prefabToInstance.reserve(instance.entities.size());
    for (const PrefabEntityLink& link : instance.entities) {
        prefabToInstance[link.prefabUuid] = link.instanceUuid;
    }

    const Scene& sourceScene = prefab.scene();
    std::uint64_t primarySourceUuid = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t firstSourceUuid = std::numeric_limits<std::uint64_t>::max();
    for (const auto [sourceHandle] :
         sourceScene.m_registry.storage<entt::entity>()->each()) {
        const std::uint64_t sourceUuid =
            sourceScene.m_registry.get<UUIDComponent>(sourceHandle).uuid;
        firstSourceUuid = std::min(firstSourceUuid, sourceUuid);
        if (!sourceScene.m_registry.all_of<ParentComponent>(sourceHandle)) {
            primarySourceUuid = std::min(primarySourceUuid, sourceUuid);
        }
    }
    if (primarySourceUuid == std::numeric_limits<std::uint64_t>::max()) {
        primarySourceUuid = firstSourceUuid;
    }
    if (primarySourceUuid != std::numeric_limits<std::uint64_t>::max()) {
        prefabToInstance[primarySourceUuid] = instanceRoot.uuid();
    }

    std::unordered_map<std::uint64_t, Entity> entities;
    std::unordered_set<std::uint64_t> remainingPrefabUuids;
    PrefabInstanceComponent updated{.prefab = instance.prefab};

    for (const auto [sourceHandle] :
         sourceScene.m_registry.storage<entt::entity>()->each()) {
        const auto& uuid = sourceScene.m_registry.get<UUIDComponent>(sourceHandle);
        const auto& tag = sourceScene.m_registry.get<TagComponent>(sourceHandle);
        remainingPrefabUuids.insert(uuid.uuid);

        Entity destination;
        if (const auto found = prefabToInstance.find(uuid.uuid);
            found != prefabToInstance.end()) {
            destination = findEntity(found->second);
        }
        if (!destination) {
            destination = createEntity(tag.tag);
        }

        const bool keepTransform = destination == instanceRoot;
        copyPrefabComponents(destination, sourceScene, sourceHandle, keepTransform);
        entities.emplace(uuid.uuid, destination);
        updated.entities.push_back({uuid.uuid, destination.uuid()});
    }

    for (const PrefabEntityLink& link : instance.entities) {
        if (remainingPrefabUuids.contains(link.prefabUuid)) {
            continue;
        }
        const Entity extra = findEntity(link.instanceUuid);
        if (extra && extra != instanceRoot) {
            destroyEntity(extra);
        }
    }

    for (const auto [sourceHandle] :
         sourceScene.m_registry.storage<entt::entity>()->each()) {
        const std::uint64_t sourceUuid =
            sourceScene.m_registry.get<UUIDComponent>(sourceHandle).uuid;
        Entity destination = entities.at(sourceUuid);
        if (const auto* relationship =
                sourceScene.m_registry.try_get<ParentComponent>(sourceHandle)) {
            setParent(destination, entities.at(relationship->parentUuid));
        } else if (destination != instanceRoot) {
            clearParent(destination);
        }
    }

    if (rootParent && valid(rootParent)) {
        setParent(instanceRoot, rootParent);
    } else {
        clearParent(instanceRoot);
    }
    instanceRoot.transform() = rootTransform;
    std::ranges::sort(
        updated.entities,
        [](const PrefabEntityLink& left, const PrefabEntityLink& right) {
            return left.prefabUuid < right.prefabUuid;
        }
    );
    instanceRoot.set<PrefabInstanceComponent>(std::move(updated));
}

void Scene::applyPrefabInstances(asset::AssetManager& assets) {
    std::vector<std::uint64_t> instanceRoots;
    std::unordered_set<asset::AssetId> prefabIds;
    for (const auto [handle, uuid, instance] : m_registry.view<
             const UUIDComponent,
             const PrefabInstanceComponent
    >().each()) {
        (void)handle;
        instanceRoots.push_back(uuid.uuid);
        if (instance.prefab.valid()) {
            prefabIds.insert(instance.prefab.handle().id());
        }
    }

    for (const asset::AssetId id : prefabIds) {
        const auto prefabHandle = asset::AssetHandle<Prefab>::fromId(id);
        if (assets.isLoaded(prefabHandle)) {
            assets.unload(prefabHandle);
        }
    }

    for (const std::uint64_t rootUuid : instanceRoots) {
        const Entity root = findEntity(rootUuid);
        if (!root || !root.has<PrefabInstanceComponent>()) {
            continue;
        }
        const auto& component = root.get<PrefabInstanceComponent>();
        if (!component.prefab.valid()) {
            continue;
        }
        try {
            const auto prefab = assets.loadResource<Prefab>(component.prefab);
            applyPrefab(root, *prefab);
        } catch (const std::exception& error) {
            ENGINE_WARN(
                "Failed to apply prefab '{}' to entity '{}': {}",
                component.prefab.sourcePath().generic_string(),
                std::string(root.name()),
                error.what()
            );
        }
    }
}

void Scene::copyPrefabComponents(
    Entity destination,
    const Scene& source,
    const entt::entity sourceHandle,
    const bool keepTransform
) {
    const auto& tag = source.m_registry.get<TagComponent>(sourceHandle);
    destination.setName(tag.tag);
    if (!keepTransform) {
        destination.component<TransformComponent>() =
            source.m_registry.get<TransformComponent>(sourceHandle);
    }

    const auto copy = [&]<typename Component>() {
        if (source.m_registry.all_of<Component>(sourceHandle)) {
            destination.set<Component>(source.m_registry.get<Component>(sourceHandle));
        } else {
            destination.remove<Component>();
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

    for (const auto& handler : SceneComponentRegistry::handlers()) {
        handler.remove(m_registry, destination.m_handle);
        handler.clone(
            source.m_registry,
            sourceHandle,
            m_registry,
            destination.m_handle
        );
    }
}

void Scene::unpackPrefab(Entity instanceRoot) {
    if (!valid(instanceRoot) || !instanceRoot.has<PrefabInstanceComponent>()) {
        throw std::invalid_argument("unpackPrefab requires a linked prefab instance root");
    }
    instanceRoot.remove<PrefabInstanceComponent>();
}

void Scene::collectDescendants(const Entity entity, std::vector<Entity>& descendants) {
    for (const Entity child : children(entity)) {
        collectDescendants(child, descendants);
        descendants.push_back(child);
    }
}

void Scene::destroyEntityInternal(const Entity entity) {
    if (!valid(entity)) {
        throw std::invalid_argument("Cannot destroy an invalid or foreign entity");
    }
    const std::uint64_t destroyedUuid = entity.uuid();
    std::vector<entt::entity> orphaned;
    for (const auto [handle, relationship] :
         m_registry.view<const ParentComponent>().each()) {
        if (relationship.parentUuid == destroyedUuid) orphaned.push_back(handle);
    }
    for (const entt::entity handle : orphaned) {
        m_registry.remove<ParentComponent>(handle);
    }
    m_entitiesByUuid.erase(destroyedUuid);
    m_registry.destroy(entity.m_handle);
}

void Scene::destroyEntity(const Entity entity) {
    if (!valid(entity)) {
        throw std::invalid_argument("Cannot destroy an invalid or foreign entity");
    }

    if (entity.has<PrefabInstanceComponent>()) {
        const PrefabInstanceComponent instance =
            entity.get<PrefabInstanceComponent>();
        std::vector<Entity> descendants;
        collectDescendants(entity, descendants);
        for (const Entity descendant : descendants) {
            if (valid(descendant)) {
                destroyEntityInternal(descendant);
            }
        }
        for (const PrefabEntityLink& link : instance.entities) {
            const Entity mapped = findEntity(link.instanceUuid);
            if (mapped && mapped != entity && valid(mapped)) {
                destroyEntityInternal(mapped);
            }
        }
    }
    destroyEntityInternal(entity);
}

void Scene::setParent(const Entity child, const Entity newParent) {
    if (!valid(child) || !valid(newParent) || child == newParent) {
        throw std::invalid_argument("Parent and child must be distinct entities in this scene");
    }
    Entity ancestor = newParent;
    while (ancestor) {
        if (ancestor == child) {
            throw std::invalid_argument("An entity hierarchy cannot contain a cycle");
        }
        ancestor = parent(ancestor);
    }
    m_registry.emplace_or_replace<ParentComponent>(
        child.m_handle,
        ParentComponent{newParent.uuid()}
    );
}

void Scene::clearParent(const Entity child) {
    if (!valid(child)) throw std::invalid_argument("Child must belong to this scene");
    m_registry.remove<ParentComponent>(child.m_handle);
}

Entity Scene::parent(const Entity child) noexcept {
    if (!valid(child)) return {};
    const auto* relationship = m_registry.try_get<ParentComponent>(child.m_handle);
    return relationship ? findEntity(relationship->parentUuid) : Entity{};
}

std::vector<Entity> Scene::children(const Entity parentEntity) {
    if (!valid(parentEntity)) {
        throw std::invalid_argument("Parent must belong to this scene");
    }
    std::vector<Entity> result;
    for (const auto [handle, uuid, relationship] :
         m_registry.view<const UUIDComponent, const ParentComponent>().each()) {
        (void)handle;
        if (relationship.parentUuid == parentEntity.uuid()) {
            result.push_back(findEntity(uuid.uuid));
        }
    }
    return result;
}

bool Scene::valid(const Entity entity) const noexcept {
    return entity.m_scene == this &&
        entity.m_sceneGeneration == m_generation &&
        m_registry.valid(entity.m_handle);
}

Entity Scene::findEntity(const std::uint64_t uuid) noexcept {
    const auto found = m_entitiesByUuid.find(uuid);
    if (found != m_entitiesByUuid.end() && m_registry.valid(found->second)) {
        return Entity(found->second, *this, m_generation);
    }
    return {};
}

Entity Scene::findEntityById(const std::uint32_t id) noexcept {
    const auto handle = static_cast<entt::entity>(id);
    if (m_registry.valid(handle)) {
        return Entity(handle, *this, m_generation);
    }
    return {};
}

const std::string& Scene::name() const noexcept {
    return m_name;
}

void Scene::setName(std::string name) {
    m_name = std::move(name);
}

const SceneEnvironment& Scene::environment() const noexcept {
    return m_environment;
}

SceneEnvironment& Scene::environment() noexcept {
    return m_environment;
}

void Scene::setEnvironment(const SceneEnvironment& environment) noexcept {
    m_environment = environment;
}

std::uint64_t Scene::generateUuid() {
    std::uint64_t candidate = 0;
    do {
        candidate = nextUuid.fetch_add(1, std::memory_order_relaxed);
    } while (candidate == 0 || findEntity(candidate));
    return candidate;
}

} // namespace vshade::scene
