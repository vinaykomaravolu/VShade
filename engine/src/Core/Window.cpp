#include "Core/Window.hpp"

#include "Core/Log.hpp"

#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <GLFW/glfw3.h>

namespace VShade {
namespace {

std::mutex glfw_mutex;
std::uint32_t window_count = 0;

void glfw_error_callback(const int code, const char* description) {
    ENGINE_ERROR("GLFW error {}: {}", code, description ? description : "unknown error");
}

int to_glfw_key(const Key key) {
    switch (key) {
        case Key::Escape: return GLFW_KEY_ESCAPE;
        case Key::Space: return GLFW_KEY_SPACE;
        case Key::A: return GLFW_KEY_A;
        case Key::D: return GLFW_KEY_D;
        case Key::S: return GLFW_KEY_S;
        case Key::W: return GLFW_KEY_W;
        case Key::Left: return GLFW_KEY_LEFT;
        case Key::Right: return GLFW_KEY_RIGHT;
        case Key::Up: return GLFW_KEY_UP;
        case Key::Down: return GLFW_KEY_DOWN;
    }

    return GLFW_KEY_UNKNOWN;
}

int checked_dimension(const std::uint32_t value, const char* name) {
    if (value == 0 || value > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument(std::string(name) + " must fit in a positive int");
    }
    return static_cast<int>(value);
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

bool Window::IsKeyPressed(const Key key) const {
    const auto state = glfwGetKey(m_impl->handle, to_glfw_key(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
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

} // namespace VShade
