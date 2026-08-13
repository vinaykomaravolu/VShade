#pragma once

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace vshade::scene {

class Scene;

/** @brief Thin, non-owning handle to an entity stored by a Scene. */
class Entity final {
public:
    /** @brief Creates an invalid entity handle. */
    Entity() noexcept;

    /** @brief Adds a component constructed from the supplied arguments. */
    template<typename Component, typename... Arguments>
    Component& addComponent(Arguments&&... arguments) {
        return registry().emplace<Component>(
            m_handle,
            std::forward<Arguments>(arguments)...
        );
    }

    /** @brief Returns a mutable component attached to this entity. */
    template<typename Component>
    [[nodiscard]] Component& component() {
        return registry().get<Component>(m_handle);
    }

    /** @brief Returns a read-only component attached to this entity. */
    template<typename Component>
    [[nodiscard]] const Component& component() const {
        return registry().get<Component>(m_handle);
    }

    /** @brief Reports whether this entity has every requested component. */
    template<typename... Components>
        requires (sizeof...(Components) > 0)
    [[nodiscard]] bool hasComponents() const {
        return valid() && registry().all_of<Components...>(m_handle);
    }

    /** @brief Removes a component and reports whether it existed. */
    template<typename Component>
    bool removeComponent() {
        return valid() && registry().remove<Component>(m_handle) != 0;
    }

    /** @brief Reports whether this handle still identifies a live entity. */
    [[nodiscard]] bool valid() const noexcept;

    /** @brief Returns the underlying EnTT identifier. */
    [[nodiscard]] entt::entity handle() const noexcept;

    /** @brief Returns the numeric portion of the EnTT identifier. */
    [[nodiscard]] std::uint32_t id() const noexcept;

    explicit operator bool() const noexcept;

    bool operator==(const Entity&) const = default;

private:
    friend class Scene;

    Entity(entt::entity handle, Scene& scene) noexcept;

    [[nodiscard]] entt::registry& registry();
    [[nodiscard]] const entt::registry& registry() const;

    entt::entity m_handle{entt::null};
    Scene* m_scene = nullptr;
};

} // namespace vshade::scene
