#include "Platform/Input.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace VShade {
namespace {

constexpr auto key_count = static_cast<std::size_t>(KeyCode::Count);
constexpr auto mouse_button_count = static_cast<std::size_t>(MouseButton::Count);

std::array<bool, key_count> keys_down{};
std::array<bool, key_count> keys_pressed{};
std::array<bool, key_count> keys_released{};

std::array<bool, mouse_button_count> mouse_buttons_down{};
std::array<bool, mouse_button_count> mouse_buttons_pressed{};
std::array<bool, mouse_button_count> mouse_buttons_released{};

glm::vec2 current_mouse_position{};
glm::vec2 current_mouse_delta{};
glm::vec2 current_scroll_delta{};
bool has_mouse_position = false;

[[nodiscard]] std::size_t index(const KeyCode key) {
    return static_cast<std::size_t>(key);
}

[[nodiscard]] std::size_t index(const MouseButton button) {
    return static_cast<std::size_t>(button);
}

[[nodiscard]] bool is_valid(const KeyCode key) {
    return key != KeyCode::Unknown && index(key) < key_count;
}

[[nodiscard]] bool is_valid(const MouseButton button) {
    return index(button) < mouse_button_count;
}

} // namespace

bool Input::is_key_down(const KeyCode key) {
    return is_valid(key) && keys_down[index(key)];
}

bool Input::is_key_pressed(const KeyCode key) {
    return is_valid(key) && keys_pressed[index(key)];
}

bool Input::is_key_released(const KeyCode key) {
    return is_valid(key) && keys_released[index(key)];
}

bool Input::is_mouse_button_down(const MouseButton button) {
    return is_valid(button) && mouse_buttons_down[index(button)];
}

bool Input::is_mouse_button_pressed(const MouseButton button) {
    return is_valid(button) && mouse_buttons_pressed[index(button)];
}

bool Input::is_mouse_button_released(const MouseButton button) {
    return is_valid(button) && mouse_buttons_released[index(button)];
}

glm::vec2 Input::mouse_position() {
    return current_mouse_position;
}

glm::vec2 Input::mouse_delta() {
    return current_mouse_delta;
}

glm::vec2 Input::scroll_delta() {
    return current_scroll_delta;
}

void Input::begin_frame() {
    std::fill(keys_pressed.begin(), keys_pressed.end(), false);
    std::fill(keys_released.begin(), keys_released.end(), false);
    std::fill(mouse_buttons_pressed.begin(), mouse_buttons_pressed.end(), false);
    std::fill(mouse_buttons_released.begin(), mouse_buttons_released.end(), false);

    current_mouse_delta = {};
    current_scroll_delta = {};
}

void Input::on_key_pressed(const KeyCode key) {
    if (!is_valid(key)) {
        return;
    }

    const auto key_index = index(key);
    keys_pressed[key_index] = !keys_down[key_index];
    keys_down[key_index] = true;
}

void Input::on_key_released(const KeyCode key) {
    if (!is_valid(key)) {
        return;
    }

    const auto key_index = index(key);
    keys_released[key_index] = keys_down[key_index];
    keys_down[key_index] = false;
}

void Input::on_mouse_button_pressed(const MouseButton button) {
    if (!is_valid(button)) {
        return;
    }

    const auto button_index = index(button);
    mouse_buttons_pressed[button_index] = !mouse_buttons_down[button_index];
    mouse_buttons_down[button_index] = true;
}

void Input::on_mouse_button_released(const MouseButton button) {
    if (!is_valid(button)) {
        return;
    }

    const auto button_index = index(button);
    mouse_buttons_released[button_index] = mouse_buttons_down[button_index];
    mouse_buttons_down[button_index] = false;
}

void Input::on_mouse_moved(const float x, const float y) {
    const glm::vec2 new_position{x, y};
    if (has_mouse_position) {
        current_mouse_delta += new_position - current_mouse_position;
    }

    current_mouse_position = new_position;
    has_mouse_position = true;
}

void Input::on_mouse_scrolled(const float x, const float y) {
    current_scroll_delta += glm::vec2{x, y};
}

} // namespace VShade
