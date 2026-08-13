#include "scene/Scene.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::scene {

Scene::Scene() = default;
Scene::~Scene() = default;

Entity Scene::createEntity(std::string name) {
    const entt::entity handle = m_registry.create();
    m_registry.emplace<TagComponent>(handle, std::move(name));
    m_registry.emplace<TransformComponent>(handle);
    return Entity(handle, *this);
}

void Scene::destroyEntity(const Entity entity) {
    if (!valid(entity)) {
        throw std::invalid_argument("Cannot destroy an invalid or foreign entity");
    }
    m_registry.destroy(entity.m_handle);
}

bool Scene::valid(const Entity entity) const noexcept {
    return entity.m_scene == this && m_registry.valid(entity.m_handle);
}

} // namespace vshade::scene
