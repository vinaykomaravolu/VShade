#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace VShade {

struct WindowConfig {
    std::string title = "VShade";
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool vertical_sync = true;
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
    explicit Window(const WindowConfig& config = {});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void poll_events() const;
    void swap_buffers() const;
    void clear(float red, float green, float blue, float alpha = 1.0F) const;

    [[nodiscard]] bool should_close() const;
    void request_close();

    [[nodiscard]] bool is_key_pressed(Key key) const;
    [[nodiscard]] std::uint32_t width() const;
    [[nodiscard]] std::uint32_t height() const;
    [[nodiscard]] void* native_handle() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace VShade
