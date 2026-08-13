#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

struct GLFWwindow;

namespace vshade::platform {

/** @brief Settings used when creating a Window. */
struct WindowConfig {
    /** @brief Initial window title. */
    std::string title = "VShade";
    /** @brief Initial client width in pixels. */
    std::uint32_t width = 1280;
    /** @brief Initial client height in pixels. */
    std::uint32_t height = 720;
    /** @brief Whether the window initially occupies the primary monitor. */
    bool fullscreen = false;
    /** @brief Whether buffer swaps initially wait for vertical synchronization. */
    bool vsync = true;
    /** @brief Whether the native window is visible after creation. */
    bool visible = true;
};

/** @brief Owns a GLFW window and its OpenGL context. */
class Window final {
public:
    /** @brief Function invoked with the new framebuffer width and height. */
    using ResizeCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    /**
     * @brief Creates a window and OpenGL context.
     * @param config Initial title, dimensions, display mode, and visibility.
     * @throws std::runtime_error If GLFW or window creation fails.
     * @throws std::invalid_argument If a requested dimension is invalid.
     */
    explicit Window(const WindowConfig& config = {});

    /** @brief Destroys the native window and releases GLFW when no windows remain. */
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /** @brief Clears transient input state and processes pending platform events. */
    void pollEvents() const;

    /** @brief Presents the current back buffer. */
    void swapBuffers() const;

    /**
     * @brief Clears the color and depth buffers with the supplied color.
     * @param red Red channel in the range 0 to 1.
     * @param green Green channel in the range 0 to 1.
     * @param blue Blue channel in the range 0 to 1.
     * @param alpha Alpha channel in the range 0 to 1.
     */
    void clear(float red, float green, float blue, float alpha = 1.0F) const;

    /**
     * @brief Returns whether the user or application requested the window to close.
     * @return True when the window should close; otherwise false.
     */
    [[nodiscard]] bool shouldClose() const;

    /** @brief Marks the window so shouldClose() returns true. */
    void requestClose();

    /**
     * @brief Replaces the callback invoked after framebuffer resize events.
     * @param callback Function that receives the new framebuffer dimensions.
     */
    void setResizeCallback(ResizeCallback callback);

    /**
     * @brief Switches between primary-monitor fullscreen and windowed mode.
     * @param fullscreen True for fullscreen mode; false for windowed mode.
     * @throws std::runtime_error If the primary monitor cannot be queried.
     */
    void setFullscreen(bool fullscreen);

    /**
     * @brief Returns whether the window is currently fullscreen.
     * @return True in fullscreen mode; otherwise false.
     */
    [[nodiscard]] bool isFullscreen() const;

    /**
     * @brief Enables or disables vertical synchronization for this context.
     * @param enabled True to synchronize buffer swaps with the display.
     */
    void setVSync(bool enabled);

    /**
     * @brief Returns whether vertical synchronization is enabled.
     * @return True when VSync is enabled; otherwise false.
     */
    [[nodiscard]] bool isVSync() const noexcept;

    /** @brief Captures or releases the cursor for unbounded mouse-look input. */
    void setCursorCaptured(bool captured);

    /** @brief Returns whether the cursor is currently captured by this window. */
    [[nodiscard]] bool isCursorCaptured() const noexcept;

    /**
     * @brief Returns the current framebuffer width in pixels.
     * @return Framebuffer width in pixels.
     */
    [[nodiscard]] std::uint32_t width() const noexcept;

    /**
     * @brief Returns the current framebuffer height in pixels.
     * @return Framebuffer height in pixels.
     */
    [[nodiscard]] std::uint32_t height() const noexcept;

    /**
     * @brief Returns the underlying GLFW handle for advanced platform integration.
     * @return Non-owning pointer to the native GLFW window.
     * @warning The pointer becomes invalid when this Window is destroyed.
     */
    [[nodiscard]] GLFWwindow* nativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::platform
