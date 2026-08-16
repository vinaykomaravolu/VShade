#pragma once

#include "asset/AssetReference.hpp"
#include "math/Vector.hpp"

#include <cstdint>
#include <filesystem>

namespace vshade::renderer {
class Model;
class Texture2D;
}

namespace vshade::scene {

/** @brief Projection model used by a scene camera. */
enum class CameraProjection {
    Perspective,
    Orthographic,
};

/** @brief Serializable camera settings attached to an entity transform. */
struct CameraComponent {
    CameraProjection projection = CameraProjection::Perspective;
    float verticalFieldOfViewRadians = 1.0471975512F;
    float orthographicHeight = 10.0F;
    float nearPlane = 0.1F;
    float farPlane = 1'000.0F;
    math::Vec4 clearColor{0.025F, 0.035F, 0.06F, 1.0F};
    std::int32_t priority = 0;
    bool active = true;
    bool clearColorEnabled = true;
    bool clearDepthEnabled = true;
};

/** @brief Serializable 3D model reference rendered at an entity transform. */
struct ModelRendererComponent {
    asset::AssetReference<renderer::Model> model;
    bool visible = true;
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
    /** @brief Preferred typed reference; texturePath remains a legacy fallback. */
    asset::AssetReference<renderer::Texture2D> texture;
};

} // namespace vshade::scene
