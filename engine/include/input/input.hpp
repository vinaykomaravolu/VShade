#pragma once

#include "input/keycode.hpp"
#include "input/mousecode.hpp"

#include <glm/vec2.hpp>

namespace vshade::platform {

class Window;

} // namespace vshade::platform

namespace vshade::input {

/**
 * @brief Exposes keyboard and mouse state collected by the main Window.
 *
 * Held state remains true until release. Pressed and released state, mouse
 * movement, and scroll movement are reset at the start of every frame.
 */
class Input final {
public:
    Input() = delete;

    /** @brief Returns whether @p key is currently held down. */
    [[nodiscard]] static bool isKeyDown(KeyCode key);

    /** @brief Returns whether @p key changed from up to down this frame. */
    [[nodiscard]] static bool isKeyPressed(KeyCode key);

    /** @brief Returns whether @p key changed from down to up this frame. */
    [[nodiscard]] static bool isKeyReleased(KeyCode key);

    /** @brief Returns whether @p button is currently held down. */
    [[nodiscard]] static bool isMouseButtonDown(MouseButton button);

    /** @brief Returns whether @p button changed from up to down this frame. */
    [[nodiscard]] static bool isMouseButtonPressed(MouseButton button);

    /** @brief Returns whether @p button changed from down to up this frame. */
    [[nodiscard]] static bool isMouseButtonReleased(MouseButton button);

    /** @brief Returns the cursor position in window coordinates. */
    [[nodiscard]] static glm::vec2 mousePosition();

    /** @brief Returns cursor movement accumulated during this frame. */
    [[nodiscard]] static glm::vec2 mouseDelta();

    /** @brief Returns scroll-wheel movement accumulated during this frame. */
    [[nodiscard]] static glm::vec2 scrollDelta();

private:
    friend class vshade::platform::Window;

    /** @brief Clears input values that last for only one frame. */
    static void beginFrame();

    /** @brief Records a platform key-press event. */
    static void onKeyPressed(KeyCode key);
    /** @brief Records a platform key-release event. */
    static void onKeyReleased(KeyCode key);

    /** @brief Records a platform mouse-button press event. */
    static void onMouseButtonPressed(MouseButton button);
    /** @brief Records a platform mouse-button release event. */
    static void onMouseButtonReleased(MouseButton button);

    /** @brief Records a platform cursor-position event. */
    static void onMouseMoved(float x, float y);
    /** @brief Records a platform scroll event. */
    static void onMouseScrolled(float x, float y);
};

} // namespace vshade::input
