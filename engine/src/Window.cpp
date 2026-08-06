#include "VShade/Window.hpp"

#include "VShade/Log.hpp"

#include <limits>
#include <mutex>
#include <stdexcept>

#include <GLFW/glfw3.h>

namespace VShade {
namespace {

std::mutex glfw_mutex;
std::uint32_t window_count = 0;

void glfw_error_callback(const int code, const char* description) {
    VSHADE_ENGINE_ERROR("GLFW error {}: {}", code, description ? description : "unknown error");
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

        m_impl->handle = glfwCreateWindow(width, height, config.title.c_str(), nullptr, nullptr);
        if (!m_impl->handle) {
            if (window_count == 0) {
                glfwTerminate();
            }
            throw std::runtime_error("Failed to create a GLFW window");
        }

        ++window_count;
    }

    glfwMakeContextCurrent(m_impl->handle);
    glfwSwapInterval(config.vertical_sync ? 1 : 0);

    VSHADE_ENGINE_INFO(
        "Created {}x{} OpenGL window '{}'",
        config.width,
        config.height,
        config.title
    );
}

Window::~Window() {
    if (!m_impl || !m_impl->handle) {
        return;
    }

    const std::scoped_lock lock(glfw_mutex);
    glfwDestroyWindow(m_impl->handle);
    m_impl->handle = nullptr;

    if (--window_count == 0) {
        glfwTerminate();
    }
}

void Window::poll_events() const {
    glfwPollEvents();
}

void Window::swap_buffers() const {
    glfwSwapBuffers(m_impl->handle);
}

void Window::clear(const float red, const float green, const float blue, const float alpha) const {
    glfwMakeContextCurrent(m_impl->handle);
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool Window::should_close() const {
    return glfwWindowShouldClose(m_impl->handle) == GLFW_TRUE;
}

void Window::request_close() {
    glfwSetWindowShouldClose(m_impl->handle, GLFW_TRUE);
}

bool Window::is_key_pressed(const Key key) const {
    const auto state = glfwGetKey(m_impl->handle, to_glfw_key(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

std::uint32_t Window::width() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_impl->handle, &width, &height);
    return width > 0 ? static_cast<std::uint32_t>(width) : 0;
}

std::uint32_t Window::height() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_impl->handle, &width, &height);
    return height > 0 ? static_cast<std::uint32_t>(height) : 0;
}

void* Window::native_handle() const noexcept {
    return m_impl->handle;
}

} // namespace VShade
