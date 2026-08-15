#pragma once

#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <script/NativeScript.hpp>

#include <functional>

namespace vshade::examples::direct3d {

/** Bridge component through which a native script controls direct game code. */
struct HelmetResetAction {
    std::function<void()> reset;
};

/** User-authored script that asks the application to reset when R is pressed. */
class HelmetResetScript final : public script::NativeScript {
public:
    void onUpdate(float) override {
        if (input::Input::isKeyPressed(input::KeyCode::R)) {
            auto& action = component<HelmetResetAction>();
            if (action.reset) action.reset();
        }
    }
};

} // namespace vshade::examples::direct3d
