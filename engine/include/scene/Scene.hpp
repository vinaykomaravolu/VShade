#pragma once

#include "scene/Components.hpp"
#include "scene/Entity.hpp"

#include <entt/entity/registry.hpp>

#include <string>

namespace vshade::scene {

/** @brief Owns all entities and components in one game scene. */
class Scene final {
public:
    Scene();
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    /** @brief Creates an entity with tag and transform components. */
    [[nodiscard]] Entity createEntity(std::string name = "Entity");

    /** @brief Destroys an entity owned by this scene. */
    void destroyEntity(Entity entity);

    /** @brief Returns an EnTT view for systems iterating selected components. */
    template<typename... Components>
    [[nodiscard]] auto view() {
        return m_registry.view<Components...>();
    }

    /** @brief Returns a read-only EnTT view for selected components. */
    template<typename... Components>
    [[nodiscard]] auto view() const {
        return m_registry.view<Components...>();
    }

    /** @brief Reports whether an entity handle belongs to this scene and is alive. */
    [[nodiscard]] bool valid(Entity entity) const noexcept;

private:
    friend class Entity;
    friend class SceneSerializer;

    entt::registry m_registry;
};

} // namespace vshade::scene
