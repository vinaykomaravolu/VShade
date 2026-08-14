#pragma once

#include "math/Transform.hpp"

#include <cstdint>
#include <string>

namespace vshade::scene {

/** @brief Stable identifier persisted independently of EnTT's runtime handle. */
struct UUIDComponent {
    std::uint64_t uuid = 0;
};

/** @brief Human-readable name attached to a scene entity. */
struct TagComponent {
    std::string tag{"Entity"};
};

/** @brief Local position, rotation, and scale attached to a scene entity. */
struct TransformComponent {
    math::Transform transform;
};

} // namespace vshade::scene
