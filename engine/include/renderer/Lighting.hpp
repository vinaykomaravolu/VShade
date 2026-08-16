#pragma once

#include "math/Vector.hpp"

#include <optional>
#include <cstddef>
#include <variant>
#include <vector>

namespace vshade::renderer {

inline constexpr std::size_t maximumDirectionalLights = 4;
inline constexpr std::size_t maximumPointLights = 16;

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

/** @brief Any positional or directional light attachable to a scene entity. */
using Light = std::variant<DirectionalLight, PointLight>;

/**
 * @brief Complete light set collected for rendering a scene.
 *
 * A scene can supply at most one ambient light and any number of directional
 * and point lights.
 */
class Lighting final {
public:
    /** @brief Creates lighting with a low-intensity white ambient light. */
    Lighting();

    [[nodiscard]] const std::optional<AmbientLight>& ambientLight() const noexcept;
    void setAmbientLight(const AmbientLight& light);
    void clearAmbientLight() noexcept;

    [[nodiscard]] const std::vector<DirectionalLight>& directionalLights() const noexcept;
    void addDirectionalLight(const DirectionalLight& light);
    void clearDirectionalLights() noexcept;

    [[nodiscard]] const std::vector<PointLight>& pointLights() const noexcept;
    void addPointLight(const PointLight& light);
    void clearPointLights() noexcept;

    /** @brief Removes every light, including ambient illumination. */
    void clear() noexcept;

private:
    std::optional<AmbientLight> m_ambientLight;
    std::vector<DirectionalLight> m_directionalLights;
    std::vector<PointLight> m_pointLights;
};

} // namespace vshade::renderer
