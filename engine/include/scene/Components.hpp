#pragma once

#include "math/transform.hpp"
#include "math/vector.hpp"

#include <cstdint>
#include <filesystem>
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

/**
 * @brief Serializable sprite description using an asset path, never a GPU pointer.
 *
 * An asset system can resolve texturePath to a Texture2D when the scene is rendered.
 */
struct SpriteRendererComponent {
    std::filesystem::path texturePath;
    math::Vec4 color{1.0F};
    math::Vec2 tiling{1.0F};
    std::int32_t sortingLayer = 0;
};

} // namespace vshade::scene
