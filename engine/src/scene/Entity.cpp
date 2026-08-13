#include "scene/Entity.hpp"

#include "scene/Scene.hpp"

#include <stdexcept>

namespace vshade::scene {

Entity::Entity() noexcept = default;

Entity::Entity(const entt::entity handle, Scene& scene) noexcept
    : m_handle(handle), m_scene(&scene) {}

bool Entity::valid() const noexcept {
    return m_scene != nullptr && m_scene->valid(*this);
}

entt::entity Entity::handle() const noexcept {
    return m_handle;
}

std::uint32_t Entity::id() const noexcept {
    return static_cast<std::uint32_t>(entt::to_integral(m_handle));
}

Entity::operator bool() const noexcept {
    return valid();
}

entt::registry& Entity::registry() {
    if (!valid()) {
        throw std::logic_error("Entity handle is not valid");
    }
    return m_scene->m_registry;
}

const entt::registry& Entity::registry() const {
    if (!valid()) {
        throw std::logic_error("Entity handle is not valid");
    }
    return m_scene->m_registry;
}

} // namespace vshade::scene
