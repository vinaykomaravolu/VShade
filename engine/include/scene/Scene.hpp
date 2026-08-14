#pragma once

#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/SceneEnvironment.hpp"

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace vshade::scene {

/** @brief Owns all entities and components in one game scene. */
class Scene final {
public:
    explicit Scene(std::string name = "Untitled");
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    /** @brief Creates an entity with tag and transform components. */
    [[nodiscard]] Entity createEntity(std::string name = "Entity");

    /** @brief Copies built-in and registered components and assigns a new UUID. */
    [[nodiscard]] Entity duplicateEntity(Entity source);

    /** @brief Destroys an entity owned by this scene. */
    void destroyEntity(Entity entity);

    /** @brief Returns an EnTT view for systems iterating selected components. */
    template<typename... Components>
    [[nodiscard]] auto view() {
        static_assert(
            (!std::same_as<Components, UUIDComponent> && ...),
            "UUIDComponent is Scene-owned and may only be viewed through const access"
        );
        return m_registry.view<Components...>();
    }

    /** @brief Returns a read-only EnTT view for selected components. */
    template<typename... Components>
    [[nodiscard]] auto view() const {
        return m_registry.view<Components...>();
    }

    /** @brief Reports whether an entity handle belongs to this scene and is alive. */
    [[nodiscard]] bool valid(Entity entity) const noexcept;

    /** @brief Finds a live entity by its stable UUID, or returns an invalid handle. */
    [[nodiscard]] Entity findEntity(std::uint64_t uuid) noexcept;

    /** @brief Returns the human-readable scene name stored in scene files. */
    [[nodiscard]] const std::string& name() const noexcept;

    /** @brief Replaces the human-readable scene name. */
    void setName(std::string name);

    /** @brief Returns the scene-wide environment lighting settings. */
    [[nodiscard]] const SceneEnvironment& environment() const noexcept;

    /** @brief Returns mutable scene-wide environment lighting settings. */
    [[nodiscard]] SceneEnvironment& environment() noexcept;

    /** @brief Replaces the scene-wide environment lighting settings. */
    void setEnvironment(const SceneEnvironment& environment) noexcept;

private:
    friend class Entity;
    friend class SceneSerializer;

    [[nodiscard]] Entity createEntityWithUuid(std::string name, std::uint64_t uuid);
    [[nodiscard]] std::uint64_t generateUuid();

    entt::registry m_registry;
    std::unordered_map<std::uint64_t, entt::entity> m_entitiesByUuid;
    std::string m_name;
    SceneEnvironment m_environment;
    std::uint64_t m_generation = 1;
};

} // namespace vshade::scene
