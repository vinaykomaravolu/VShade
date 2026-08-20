#include "scene/Entity.hpp"

#include "scene/Scene.hpp"

#include <stdexcept>

namespace vshade::scene {

Entity::Entity() noexcept = default;

math::Transform& Entity::transform() {
    return component<TransformComponent>().transform;
}

const math::Transform& Entity::transform() const {
    return component<TransformComponent>().transform;
}

std::string_view Entity::name() const {
    return component<TagComponent>().tag;
}

void Entity::setName(std::string name) {
    if (name.empty()) {
        throw std::invalid_argument("An entity name cannot be empty");
    }
    component<TagComponent>().tag = std::move(name);
}

Entity::Entity(
    const entt::entity handle,
    Scene& scene,
    const std::uint64_t sceneGeneration
) noexcept
    : m_handle(handle),
      m_scene(&scene),
      m_sceneGeneration(sceneGeneration) {}

Scene& Entity::scene() {
    if (!valid()) {
        throw std::logic_error("Entity handle is not valid");
    }
    return *m_scene;
}

const Scene& Entity::scene() const {
    if (!valid()) {
        throw std::logic_error("Entity handle is not valid");
    }
    return *m_scene;
}

bool Entity::valid() const noexcept {
    return m_scene != nullptr && m_scene->valid(*this);
}

entt::entity Entity::handle() const noexcept {
    return m_handle;
}

std::uint32_t Entity::id() const noexcept {
    return static_cast<std::uint32_t>(entt::to_integral(m_handle));
}

std::uint64_t Entity::uuid() const {
    return component<UUIDComponent>().uuid;
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
