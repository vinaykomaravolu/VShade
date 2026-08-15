#pragma once

#include <math/Quaternion.hpp>
#include <script/NativeScript.hpp>

namespace vshade::examples::scripts {

/** Rotates the entity that owns this script. */
class SpinScript final : public script::NativeScript {
public:
    void onUpdate(const float deltaTime) override {
        m_angle += radiansPerSecond * deltaTime;
        entity().transform().setRotation(
            math::fromEuler({0.0F, 0.0F, m_angle})
        );
    }

    float radiansPerSecond = 1.5F;

private:
    float m_angle = 0.0F;
};

} // namespace vshade::examples::scripts
