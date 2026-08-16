#pragma once

#include "renderer/Lighting.hpp"

namespace vshade::scene {

class Scene;

/** @brief Collects scene environment and entity lights into renderer world data. */
class SceneLightingSystem final {
public:
    SceneLightingSystem() = delete;

    /**
     * @brief Builds the complete world-space lighting set for a scene.
     *
     * Directional vectors and point-light offsets are stored in entity-local
     * space. Entity rotation transforms both, while entity position moves point
     * lights into world space. Disabled LightComponents are ignored.
     */
    [[nodiscard]] static renderer::Lighting collect(const Scene& scene);
};

} // namespace vshade::scene
