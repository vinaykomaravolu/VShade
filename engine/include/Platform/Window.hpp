#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

struct GLFWwindow;

namespace VShade {

struct WindowConfig {
    std::string title = "VShade";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool fullscreen = false;
    bool vsync = true;
};

enum class Key {
    Escape,
    Space,
    A,
    D,
    S,
    W,
    Left,
    Right,
    Up,
    Down
};

class Window final {
public:
    using ResizeCallback = std::function<void(std::uint32_t, std::uint32_t)>;

    explicit Window(const WindowConfig& config = {});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void PollEvents() const;
    void SwapBuffers() const;
    void Clear(float red, float green, float blue, float alpha = 1.0F) const;

    [[nodiscard]] bool ShouldClose() const;
    void RequestClose();

    void SetResizeCallback(ResizeCallback callback);
    void SetFullscreen(bool fullscreen);
    [[nodiscard]] bool IsFullscreen() const;

    void SetVSync(bool enabled);
    [[nodiscard]] bool IsVSync() const noexcept;

    [[nodiscard]] bool IsKeyPressed(Key key) const;
    [[nodiscard]] std::uint32_t Width() const noexcept;
    [[nodiscard]] std::uint32_t Height() const noexcept;
    [[nodiscard]] GLFWwindow* NativeHandle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace VShade
