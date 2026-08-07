#pragma once

#include "platform/keycode.hpp"
#include "platform/mousecode.hpp"

#include <glm/vec2.hpp>

namespace vshade::platform {

class Window;

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
    [[nodiscard]] static bool is_key_down(KeyCode key);

    /** @brief Returns whether @p key changed from up to down this frame. */
    [[nodiscard]] static bool is_key_pressed(KeyCode key);

    /** @brief Returns whether @p key changed from down to up this frame. */
    [[nodiscard]] static bool is_key_released(KeyCode key);

    /** @brief Returns whether @p button is currently held down. */
    [[nodiscard]] static bool is_mouse_button_down(MouseButton button);

    /** @brief Returns whether @p button changed from up to down this frame. */
    [[nodiscard]] static bool is_mouse_button_pressed(MouseButton button);

    /** @brief Returns whether @p button changed from down to up this frame. */
    [[nodiscard]] static bool is_mouse_button_released(MouseButton button);

    /** @brief Returns the cursor position in window coordinates. */
    [[nodiscard]] static glm::vec2 mouse_position();

    /** @brief Returns cursor movement accumulated during this frame. */
    [[nodiscard]] static glm::vec2 mouse_delta();

    /** @brief Returns scroll-wheel movement accumulated during this frame. */
    [[nodiscard]] static glm::vec2 scroll_delta();

private:
    friend class Window;

    /** @brief Clears input values that last for only one frame. */
    static void begin_frame();

    /** @brief Records a platform key-press event. */
    static void on_key_pressed(KeyCode key);
    /** @brief Records a platform key-release event. */
    static void on_key_released(KeyCode key);

    /** @brief Records a platform mouse-button press event. */
    static void on_mouse_button_pressed(MouseButton button);
    /** @brief Records a platform mouse-button release event. */
    static void on_mouse_button_released(MouseButton button);

    /** @brief Records a platform cursor-position event. */
    static void on_mouse_moved(float x, float y);
    /** @brief Records a platform scroll event. */
    static void on_mouse_scrolled(float x, float y);
};

} // namespace vshade::platform
