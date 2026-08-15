#pragma once

#include <scene/Components.hpp>
#include <script/NativeScript.hpp>

#include <cmath>

namespace vshade::tests::assets {

/** @brief Example user-authored script that oscillates its entity vertically. */
class Bounce final : public script::NativeScript {
public:
    void onCreate() override {
        initialPosition = component<scene::TransformComponent>().transform.position();
    }

    void onUpdate(const float deltaTime) override {
        elapsedTime += deltaTime;
        auto& transform = component<scene::TransformComponent>().transform;
        math::Vec3 position = initialPosition;
        position.y += amplitude * std::sin(elapsedTime * angularFrequency);
        transform.setPosition(position);
    }

    float amplitude = 0.5F;
    float angularFrequency = 2.0F;

private:
    math::Vec3 initialPosition{0.0F};
    float elapsedTime = 0.0F;
};

} // namespace vshade::tests::assets
