#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

struct GLFWwindow;

namespace VShade {

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
};

/** @brief Owns a GLFW window and its OpenGL context. */
class Window final {
public:
    /** @brief Function invoked with the new framebuffer width and height. */
    using ResizeCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    /**
     * @brief Creates a window and OpenGL context.
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
    void PollEvents() const;

    /** @brief Presents the current back buffer. */
    void SwapBuffers() const;

    /**
     * @brief Clears the color and depth buffers with the supplied color.
     * @param red Red channel in the range 0 to 1.
     * @param green Green channel in the range 0 to 1.
     * @param blue Blue channel in the range 0 to 1.
     * @param alpha Alpha channel in the range 0 to 1.
     */
    void Clear(float red, float green, float blue, float alpha = 1.0F) const;

    /** @brief Returns whether the user or application requested the window to close. */
    [[nodiscard]] bool ShouldClose() const;

    /** @brief Marks the window so ShouldClose() returns true. */
    void RequestClose();

    /** @brief Replaces the callback invoked after framebuffer resize events. */
    void SetResizeCallback(ResizeCallback callback);

    /** @brief Switches between primary-monitor fullscreen and windowed mode. */
    void SetFullscreen(bool fullscreen);

    /** @brief Returns whether the window is currently fullscreen. */
    [[nodiscard]] bool IsFullscreen() const;

    /** @brief Enables or disables vertical synchronization for this context. */
    void SetVSync(bool enabled);

    /** @brief Returns whether vertical synchronization is enabled. */
    [[nodiscard]] bool IsVSync() const noexcept;

    /** @brief Returns the current framebuffer width in pixels. */
    [[nodiscard]] std::uint32_t Width() const noexcept;

    /** @brief Returns the current framebuffer height in pixels. */
    [[nodiscard]] std::uint32_t Height() const noexcept;

    /** @brief Returns the underlying GLFW handle for advanced platform integration. */
    [[nodiscard]] GLFWwindow* NativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace VShade
