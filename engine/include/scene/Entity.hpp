#pragma once

#include "scene/Components.hpp"

#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace vshade::scene {

namespace detail {

template<typename Component>
inline constexpr bool requiredComponent =
    std::same_as<std::remove_cv_t<Component>, UUIDComponent> ||
    std::same_as<std::remove_cv_t<Component>, TagComponent> ||
    std::same_as<std::remove_cv_t<Component>, TransformComponent>;
} // namespace detail

class Scene;

/** @brief Thin, non-owning handle to an entity stored by a Scene. */
class Entity final {
public:
    /** @brief Creates an invalid entity handle. */
    Entity() noexcept;

    /** @brief Adds a component constructed from the supplied arguments. */
    template<typename Component, typename... Arguments>
    Component& addComponent(Arguments&&... arguments) {
        static_assert(
            !detail::requiredComponent<Component>,
            "UUID, tag, and transform components are owned by Scene"
        );
        return registry().emplace<Component>(
            m_handle,
            std::forward<Arguments>(arguments)...
        );
    }

    /** @brief Concise alias for addComponent(). */
    template<typename Component, typename... Arguments>
    Component& add(Arguments&&... arguments) {
        return addComponent<Component>(std::forward<Arguments>(arguments)...);
    }

    /** @brief Adds or replaces a component in one operation. */
    template<typename Component, typename... Arguments>
    Component& set(Arguments&&... arguments) {
        static_assert(
            !std::same_as<std::remove_cv_t<Component>, UUIDComponent>,
            "An entity UUID cannot be replaced"
        );
        return registry().emplace_or_replace<Component>(
            m_handle,
            std::forward<Arguments>(arguments)...
        );
    }

    /** @brief Returns a mutable component attached to this entity. */
    template<typename Component>
        requires (!std::same_as<std::remove_cv_t<Component>, UUIDComponent>)
    [[nodiscard]] Component& component() {
        return registry().get<Component>(m_handle);
    }

    /** @brief Returns a read-only component attached to this entity. */
    template<typename Component>
    [[nodiscard]] const Component& component() const {
        return registry().get<Component>(m_handle);
    }

    /** @brief Concise alias for component(). */
    template<typename Component>
        requires (!std::same_as<std::remove_cv_t<Component>, UUIDComponent>)
    [[nodiscard]] Component& get() {
        return component<Component>();
    }

    /** @brief Concise const alias for component(). */
    template<typename Component>
    [[nodiscard]] const Component& get() const {
        return component<Component>();
    }

    /** @brief Returns a component pointer, or null when it is absent. */
    template<typename Component>
    [[nodiscard]] Component* tryGet() noexcept {
        if (!valid()) {
            return nullptr;
        }
        return registry().try_get<Component>(m_handle);
    }

    /** @brief Returns a read-only component pointer, or null when absent. */
    template<typename Component>
    [[nodiscard]] const Component* tryGet() const noexcept {
        if (!valid()) {
            return nullptr;
        }
        return registry().try_get<Component>(m_handle);
    }

    /** @brief Reports whether this entity has every requested component. */
    template<typename... Components>
        requires (sizeof...(Components) > 0)
    [[nodiscard]] bool hasComponents() const {
        return valid() && registry().all_of<Components...>(m_handle);
    }

    /** @brief Concise alias for hasComponents(). */
    template<typename... Components>
        requires (sizeof...(Components) > 0)
    [[nodiscard]] bool has() const {
        return hasComponents<Components...>();
    }

    /** @brief Removes a component and reports whether it existed. */
    template<typename Component>
    bool removeComponent() {
        static_assert(
            !detail::requiredComponent<Component>,
            "UUID, tag, and transform components cannot be removed"
        );
        return valid() && registry().remove<Component>(m_handle) != 0;
    }

    /** @brief Concise alias for removeComponent(). */
    template<typename Component>
    bool remove() {
        return removeComponent<Component>();
    }

    /** @brief Direct access to the transform present on every entity. */
    [[nodiscard]] math::Transform& transform();

    /** @brief Direct read-only access to the entity transform. */
    [[nodiscard]] const math::Transform& transform() const;

    /** @brief Returns the human-readable entity name. */
    [[nodiscard]] std::string_view name() const;

    /** @brief Changes the human-readable entity name. */
    void setName(std::string name);

    /** @brief Reports whether this handle still identifies a live entity. */
    [[nodiscard]] bool valid() const noexcept;

    /** @brief Returns the underlying EnTT identifier. */
    [[nodiscard]] entt::entity handle() const noexcept;

    /** @brief Returns the numeric portion of the EnTT identifier. */
    [[nodiscard]] std::uint32_t id() const noexcept;

    /** @brief Returns the stable identifier saved with this entity. */
    [[nodiscard]] std::uint64_t uuid() const;

    explicit operator bool() const noexcept;

    bool operator==(const Entity&) const = default;

private:
    friend class Scene;

    Entity(entt::entity handle, Scene& scene, std::uint64_t sceneGeneration) noexcept;

    [[nodiscard]] entt::registry& registry();
    [[nodiscard]] const entt::registry& registry() const;

    entt::entity m_handle{entt::null};
    Scene* m_scene = nullptr;
    std::uint64_t m_sceneGeneration = 0;
};

} // namespace vshade::scene
