#pragma once

#include "renderer/Lighting.hpp"

namespace vshade::scene {

/** @brief Attaches an ambient, directional, or point light to an entity. */
struct LightComponent {
    renderer::Light light{renderer::PointLight{}};
    bool enabled = true;
};

} // namespace vshade::scene
