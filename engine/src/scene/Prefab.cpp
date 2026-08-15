#include "scene/Prefab.hpp"

#include "scene/Scene.hpp"

#include <stdexcept>

namespace vshade::scene {

Prefab::Prefab(std::shared_ptr<const Scene> scene)
    : m_scene(std::move(scene)) {
    if (!m_scene) throw std::invalid_argument("A prefab scene cannot be null");
}

const Scene& Prefab::scene() const noexcept { return *m_scene; }

} // namespace vshade::scene
