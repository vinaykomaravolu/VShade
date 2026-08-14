#pragma once

#include "math/Vector.hpp"

#include <cstdint>
#include <filesystem>

namespace vshade::scene {

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
