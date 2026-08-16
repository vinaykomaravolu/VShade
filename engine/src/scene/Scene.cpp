#include "scene/Scene.hpp"
#include "scene/SceneComponentRegistry.hpp"
#include "scene/Prefab.hpp"

#include <atomic>
#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>
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
        copyComponent.template operator()<ScriptComponent>();
        copyComponent.template operator()<ParentComponent>();

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
            destination.component<TransformComponent>() =
                sourceScene.m_registry.get<TransformComponent>(sourceHandle);

            const auto copy = [&]<typename Component>() {
                if (sourceScene.m_registry.all_of<Component>(sourceHandle)) {
                    destination.addComponent<Component>(
                        sourceScene.m_registry.get<Component>(sourceHandle)
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
            copy.template operator()<ScriptComponent>();

            if (const auto* relationship =
                sourceScene.m_registry.try_get<ParentComponent>(sourceHandle)) {
                destination.addComponent<ParentComponent>(ParentComponent{
                    entities.at(relationship->parentUuid).uuid()
                });
            }
            for (const auto& handler : SceneComponentRegistry::handlers()) {
                handler.clone(
                    sourceScene.m_registry,
                    sourceHandle,
                    m_registry,
                    destination.m_handle
                );
            }
        }
    } catch (...) {
        for (auto iterator = created.rbegin(); iterator != created.rend(); ++iterator) {
            if (valid(*iterator)) destroyEntity(*iterator);
        }
        throw;
    }
    if (primarySourceUuid == std::numeric_limits<std::uint64_t>::max()) {
        primarySourceUuid = firstSourceUuid;
    }
    return primarySourceUuid == std::numeric_limits<std::uint64_t>::max()
        ? Entity{}
        : entities.at(primarySourceUuid);
}

void Scene::destroyEntity(const Entity entity) {
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
