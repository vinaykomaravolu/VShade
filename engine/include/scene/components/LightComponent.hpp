#pragma once

#include "renderer/Lighting.hpp"

namespace vshade::scene {

/** @brief Attaches a directional or point light to an entity. */
struct LightComponent {
    renderer::Light light{renderer::PointLight{}};
    bool enabled = true;
};

} // namespace vshade::scene
