#pragma once

#include "input/KeyCode.hpp"
#include "input/MouseCode.hpp"

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

    /**
     * @brief Returns whether @p key is currently held down.
     * @param key Keyboard key to query.
     * @return True while the key is held; otherwise false.
     */
    [[nodiscard]] static bool isKeyDown(KeyCode key);

    /**
     * @brief Returns whether @p key changed from up to down this frame.
     * @param key Keyboard key to query.
     * @return True only on the frame the key was pressed.
     */
    [[nodiscard]] static bool isKeyPressed(KeyCode key);

    /**
     * @brief Returns whether @p key changed from down to up this frame.
     * @param key Keyboard key to query.
     * @return True only on the frame the key was released.
     */
    [[nodiscard]] static bool isKeyReleased(KeyCode key);

    /**
     * @brief Returns whether @p button is currently held down.
     * @param button Mouse button to query.
     * @return True while the button is held; otherwise false.
     */
    [[nodiscard]] static bool isMouseButtonDown(MouseButton button);

    /**
     * @brief Returns whether @p button changed from up to down this frame.
     * @param button Mouse button to query.
     * @return True only on the frame the button was pressed.
     */
    [[nodiscard]] static bool isMouseButtonPressed(MouseButton button);

    /**
     * @brief Returns whether @p button changed from down to up this frame.
     * @param button Mouse button to query.
     * @return True only on the frame the button was released.
     */
    [[nodiscard]] static bool isMouseButtonReleased(MouseButton button);

    /**
     * @brief Returns the cursor position in window coordinates.
     * @return Current cursor position in pixels.
     */
    [[nodiscard]] static glm::vec2 mousePosition();

    /**
     * @brief Returns cursor movement accumulated during this frame.
     * @return Cursor movement in pixels since the previous frame.
     */
    [[nodiscard]] static glm::vec2 mouseDelta();

    /**
     * @brief Returns scroll-wheel movement accumulated during this frame.
     * @return Horizontal and vertical scroll offsets for the current frame.
     */
    [[nodiscard]] static glm::vec2 scrollDelta();

private:
    friend class vshade::platform::Window;

    /** @brief Clears input values that last for only one frame. */
    static void beginFrame();

    /**
     * @brief Records a platform key-press event.
     * @param key Key reported by the platform callback.
     */
    static void onKeyPressed(KeyCode key);
    /**
     * @brief Records a platform key-release event.
     * @param key Key reported by the platform callback.
     */
    static void onKeyReleased(KeyCode key);

    /**
     * @brief Records a platform mouse-button press event.
     * @param button Mouse button reported by the platform callback.
     */
    static void onMouseButtonPressed(MouseButton button);
    /**
     * @brief Records a platform mouse-button release event.
     * @param button Mouse button reported by the platform callback.
     */
    static void onMouseButtonReleased(MouseButton button);

    /**
     * @brief Records a platform cursor-position event.
     * @param x Cursor position along the window's horizontal axis.
     * @param y Cursor position along the window's vertical axis.
     */
    static void onMouseMoved(float x, float y);
    /**
     * @brief Records a platform scroll event.
     * @param x Horizontal scroll amount.
     * @param y Vertical scroll amount.
     */
    static void onMouseScrolled(float x, float y);

    /** @brief Clears all held and transient input after focus or window changes. */
    static void reset();

    /** @brief Prevents a synthetic cursor jump after changing cursor mode. */
    static void resetMouseTracking();
};

} // namespace vshade::input
