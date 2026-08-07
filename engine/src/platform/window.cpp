#include "platform/window.hpp"

#include "core/log.hpp"
#include "platform/input.hpp"

#include <limits>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

#include <GLFW/glfw3.h>

namespace vshade::platform {
namespace {

std::mutex glfw_mutex;
std::uint32_t window_count = 0;

void glfw_error_callback(const int code, const char* description) {
    ENGINE_ERROR("GLFW error {}: {}", code, description ? description : "unknown error");
}

int checked_dimension(const std::uint32_t value, const char* name) {
    if (value == 0 || value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument(std::string(name) + " must fit in a positive int");
    }
    return static_cast<int>(value);
}

std::optional<KeyCode> to_key_code(const int key) {
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        return static_cast<KeyCode>(
            static_cast<int>(KeyCode::D0) + (key - GLFW_KEY_0)
        );
    }

    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
        return static_cast<KeyCode>(
            static_cast<int>(KeyCode::A) + (key - GLFW_KEY_A)
        );
    }

    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
        return static_cast<KeyCode>(
            static_cast<int>(KeyCode::F1) + (key - GLFW_KEY_F1)
        );
    }

    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
        return static_cast<KeyCode>(
            static_cast<int>(KeyCode::Keypad0) + (key - GLFW_KEY_KP_0)
        );
    }

    switch (key) {
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
        case GLFW_KEY_COMMA: return KeyCode::Comma;
        case GLFW_KEY_MINUS: return KeyCode::Minus;
        case GLFW_KEY_PERIOD: return KeyCode::Period;
        case GLFW_KEY_SLASH: return KeyCode::Slash;
        case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
        case GLFW_KEY_EQUAL: return KeyCode::Equal;
        case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
        case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;
        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
        case GLFW_KEY_INSERT: return KeyCode::Insert;
        case GLFW_KEY_DELETE: return KeyCode::Delete;
        case GLFW_KEY_RIGHT: return KeyCode::Right;
        case GLFW_KEY_LEFT: return KeyCode::Left;
        case GLFW_KEY_DOWN: return KeyCode::Down;
        case GLFW_KEY_UP: return KeyCode::Up;
        case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
        case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
        case GLFW_KEY_HOME: return KeyCode::Home;
        case GLFW_KEY_END: return KeyCode::End;
        case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
        case GLFW_KEY_NUM_LOCK: return KeyCode::NumLock;
        case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
        case GLFW_KEY_PAUSE: return KeyCode::Pause;
        case GLFW_KEY_KP_DECIMAL: return KeyCode::KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return KeyCode::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return KeyCode::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return KeyCode::KeypadSubtract;
        case GLFW_KEY_KP_ADD: return KeyCode::KeypadAdd;
        case GLFW_KEY_KP_ENTER: return KeyCode::KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return KeyCode::KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;
        case GLFW_KEY_MENU: return KeyCode::Menu;
        default: return std::nullopt;
    }
}

std::optional<MouseButton> to_mouse_button(const int button) {
    if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_8) {
        return std::nullopt;
    }

    return static_cast<MouseButton>(button - GLFW_MOUSE_BUTTON_1);
}

} // namespace

struct Window::Impl {
    GLFWwindow* handle = nullptr;
    ResizeCallback resize_callback;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool fullscreen = false;
    bool vsync = true;
    int windowed_x = 100;
    int windowed_y = 100;
    int windowed_width = 1280;
    int windowed_height = 720;
};

Window::Window(const WindowConfig& config)
    : m_impl(std::make_unique<Impl>()) {
    const auto width = checked_dimension(config.width, "Window width");
    const auto height = checked_dimension(config.height, "Window height");

    {
        const std::scoped_lock lock(glfw_mutex);

        if (window_count == 0) {
            glfwSetErrorCallback(glfw_error_callback);
            if (glfwInit() != GLFW_TRUE) {
                throw std::runtime_error("Failed to initialize GLFW");
            }
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

        GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        if (config.fullscreen && !monitor) {
            if (window_count == 0) {
                glfwTerminate();
            }
            throw std::runtime_error("Failed to find the primary monitor");
        }

        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (!mode) {
                if (window_count == 0) {
                    glfwTerminate();
                }
                throw std::runtime_error("Failed to read the primary monitor video mode");
            }
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }

        m_impl->handle = glfwCreateWindow(width, height, config.title.c_str(), monitor, nullptr);
        if (!m_impl->handle) {
            if (window_count == 0) {
                glfwTerminate();
            }
            throw std::runtime_error("Failed to create a GLFW window");
        }

        ++window_count;
    }

    m_impl->fullscreen = config.fullscreen;
    m_impl->vsync = config.vsync;
    m_impl->windowed_width = width;
    m_impl->windowed_height = height;

    glfwSetWindowUserPointer(m_impl->handle, m_impl.get());
    glfwSetFramebufferSizeCallback(m_impl->handle, [](GLFWwindow* handle, const int new_width, const int new_height) {
        auto* impl = static_cast<Impl*>(glfwGetWindowUserPointer(handle));
        if (!impl) {
            return;
        }

        impl->width = new_width > 0 ? static_cast<std::uint32_t>(new_width) : 0;
        impl->height = new_height > 0 ? static_cast<std::uint32_t>(new_height) : 0;
        if (impl->resize_callback) {
            impl->resize_callback(impl->width, impl->height);
        }
    });

    glfwSetKeyCallback(
        m_impl->handle,
        [](GLFWwindow*, const int key, const int scancode, const int action, const int modifiers) {
            static_cast<void>(scancode);
            static_cast<void>(modifiers);

            const auto key_code = to_key_code(key);
            if (!key_code) {
                return;
            }

            if (action == GLFW_PRESS) {
                Input::on_key_pressed(*key_code);
            } else if (action == GLFW_RELEASE) {
                Input::on_key_released(*key_code);
            }
        }
    );

    glfwSetMouseButtonCallback(
        m_impl->handle,
        [](GLFWwindow*, const int button, const int action, const int modifiers) {
            static_cast<void>(modifiers);

            const auto mouse_button = to_mouse_button(button);
            if (!mouse_button) {
                return;
            }

            if (action == GLFW_PRESS) {
                Input::on_mouse_button_pressed(*mouse_button);
            } else if (action == GLFW_RELEASE) {
                Input::on_mouse_button_released(*mouse_button);
            }
        }
    );

    glfwSetCursorPosCallback(m_impl->handle, [](GLFWwindow*, const double x, const double y) {
        Input::on_mouse_moved(static_cast<float>(x), static_cast<float>(y));
    });

    glfwSetScrollCallback(m_impl->handle, [](GLFWwindow*, const double x, const double y) {
        Input::on_mouse_scrolled(static_cast<float>(x), static_cast<float>(y));
    });

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(m_impl->handle, &framebuffer_width, &framebuffer_height);
    m_impl->width = framebuffer_width > 0 ? static_cast<std::uint32_t>(framebuffer_width) : 0;
    m_impl->height = framebuffer_height > 0 ? static_cast<std::uint32_t>(framebuffer_height) : 0;

    glfwMakeContextCurrent(m_impl->handle);
    glfwSwapInterval(config.vsync ? 1 : 0);

    ENGINE_INFO(
        "Created {}x{} OpenGL window '{}'",
        m_impl->width,
        m_impl->height,
        config.title
    );
}

Window::~Window() {
    if (!m_impl || !m_impl->handle) {
        return;
    }

    const std::scoped_lock lock(glfw_mutex);
    glfwSetWindowUserPointer(m_impl->handle, nullptr);
    glfwDestroyWindow(m_impl->handle);
    m_impl->handle = nullptr;

    if (--window_count == 0) {
        glfwTerminate();
    }
}

void Window::PollEvents() const {
    Input::begin_frame();
    glfwPollEvents();
}

void Window::SwapBuffers() const {
    glfwSwapBuffers(m_impl->handle);
}

void Window::Clear(const float red, const float green, const float blue, const float alpha) const {
    glfwMakeContextCurrent(m_impl->handle);
    glViewport(0, 0, static_cast<int>(m_impl->width), static_cast<int>(m_impl->height));
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_impl->handle) == GLFW_TRUE;
}

void Window::RequestClose() {
    glfwSetWindowShouldClose(m_impl->handle, GLFW_TRUE);
}

void Window::SetResizeCallback(ResizeCallback callback) {
    m_impl->resize_callback = std::move(callback);
}

void Window::SetFullscreen(const bool fullscreen) {
    if (fullscreen == m_impl->fullscreen) {
        return;
    }

    if (fullscreen) {
        glfwGetWindowPos(m_impl->handle, &m_impl->windowed_x, &m_impl->windowed_y);
        glfwGetWindowSize(m_impl->handle, &m_impl->windowed_width, &m_impl->windowed_height);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (!monitor || !mode) {
            throw std::runtime_error("Failed to enter fullscreen mode");
        }

        glfwSetWindowMonitor(
            m_impl->handle,
            monitor,
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate
        );
    } else {
        glfwSetWindowMonitor(
            m_impl->handle,
            nullptr,
            m_impl->windowed_x,
            m_impl->windowed_y,
            m_impl->windowed_width,
            m_impl->windowed_height,
            GLFW_DONT_CARE
        );
    }

    m_impl->fullscreen = fullscreen;
}

bool Window::IsFullscreen() const {
    return m_impl->fullscreen;
}

void Window::SetVSync(const bool enabled) {
    glfwMakeContextCurrent(m_impl->handle);
    glfwSwapInterval(enabled ? 1 : 0);
    m_impl->vsync = enabled;
}

bool Window::IsVSync() const noexcept {
    return m_impl->vsync;
}

std::uint32_t Window::Width() const noexcept {
    return m_impl->width;
}

std::uint32_t Window::Height() const noexcept {
    return m_impl->height;
}

GLFWwindow* Window::NativeHandle() const noexcept {
    return m_impl->handle;
}

} // namespace vshade::platform
