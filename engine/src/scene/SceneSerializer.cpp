#include "scene/SceneSerializer.hpp"

#include <stdexcept>

namespace vshade::scene {

SceneSerializer::SceneSerializer(Scene& scene) noexcept
    : m_scene(scene) {}

bool SceneSerializer::serialize(const std::filesystem::path&) const {
    throw std::logic_error("Scene serialization format is not implemented yet");
}

bool SceneSerializer::deserialize(const std::filesystem::path&) {
    throw std::logic_error("Scene serialization format is not implemented yet");
}

} // namespace vshade::scene
