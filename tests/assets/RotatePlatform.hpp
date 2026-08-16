#pragma once

#include <math/Quaternion.hpp>
#include <scene/Components.hpp>
#include <script/NativeScript.hpp>

namespace vshade::tests::assets {

/** @brief Example user-authored script that rotates its entity every frame. */
class RotatePlatform final : public script::NativeScript {
public:
    void onUpdate(const float deltaTime) override {
        auto& transform = component<scene::TransformComponent>().transform;
        transform.rotate(math::fromAxisAngle(axis, radiansPerSecond * deltaTime));
    }

    math::Vec3 axis{0.0F, 0.0F, 1.0F};
    float radiansPerSecond = 1.0F;
};

} // namespace vshade::tests::assets
