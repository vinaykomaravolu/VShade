#pragma once

#include <math/Vector.hpp>
#include <script/NativeScript.hpp>

#include <cmath>

namespace vshade::examples::scripts {

/** Remembers its starting position and oscillates vertically around it. */
class BounceScript final : public script::NativeScript {
public:
    void onCreate() override {
        m_startPosition = entity().transform().position();
    }

    void onUpdate(const float deltaTime) override {
        m_elapsedTime += deltaTime;
        math::Vec3 position = m_startPosition;
        position.y += amplitude * std::sin(m_elapsedTime * angularFrequency);
        entity().transform().setPosition(position);
    }

    float amplitude = 0.75F;
    float angularFrequency = 2.5F;

private:
    math::Vec3 m_startPosition{0.0F};
    float m_elapsedTime = 0.0F;
};

} // namespace vshade::examples::scripts
