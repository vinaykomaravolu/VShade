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

/** @brief Optional stable parent relationship; transforms remain local-space data. */
struct ParentComponent {
    std::uint64_t parentUuid = 0;
};

/** @brief Optional persistent ordering key used among hierarchy siblings. */
struct HierarchyOrderComponent {
    std::int64_t siblingOrder = 0;
};

/** @brief Persistent hierarchy visibility and editor-lock state. */
struct HierarchyStateComponent {
    bool visible = true;
    bool locked = false;
};

} // namespace vshade::scene
