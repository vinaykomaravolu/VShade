#pragma once

#include "math/Vector.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace vshade::renderer {

/** @brief Uniform, direction-independent illumination for a scene. */
struct AmbientLight {
    /** @brief Red, green, and blue light color. */
    math::Vec3 color{1.0F};
    /** @brief Scalar brightness multiplier. */
    float intensity = 0.1F;
};

/** @brief Light whose parallel rays travel in one world-space direction. */
struct DirectionalLight {
    /** @brief World-space direction in which the light rays travel. */
    math::Vec3 direction{-0.5F, -1.0F, -0.25F};
    /** @brief Red, green, and blue light color. */
    math::Vec3 color{1.0F};
    /** @brief Scalar brightness multiplier. */
    float intensity = 1.0F;
};

/** @brief Light emitted from one world-space position over a finite range. */
struct PointLight {
    /** @brief World-space origin of the light. */
    math::Vec3 position{0.0F};
    /** @brief Red, green, and blue light color. */
    math::Vec3 color{1.0F};
    /** @brief Scalar brightness multiplier. */
    float intensity = 1.0F;
    /** @brief Maximum world-space distance reached by the light. */
    float range = 10.0F;
};

/** @brief Any light type that can be attached to a scene entity. */
using Light = std::variant<AmbientLight, DirectionalLight, PointLight>;

/**
 * @brief Complete light set collected for rendering a scene.
 *
 * A scene can supply at most one ambient light and any number of directional
 * and point lights.
 */
struct Lighting {
    std::optional<AmbientLight> ambientLight;
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
};

} // namespace vshade::renderer
