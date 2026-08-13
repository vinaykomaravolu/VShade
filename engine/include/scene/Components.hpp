#pragma once

#include "math/transform.hpp"

#include <string>

namespace vshade::scene {

/** @brief Human-readable name attached to a scene entity. */
struct TagComponent {
    std::string tag{"Entity"};
};

/** @brief Local position, rotation, and scale attached to a scene entity. */
struct TransformComponent {
    math::Transform transform;
};

} // namespace vshade::scene
