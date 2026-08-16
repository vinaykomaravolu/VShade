#pragma once

#include "math/Vector.hpp"

namespace vshade::scene {

/** @brief Global, non-positional illumination settings owned by one scene. */
struct SceneEnvironment {
    math::Vec3 ambientColor{1.0F};
    float ambientIntensity = 0.15F;
};

} // namespace vshade::scene
