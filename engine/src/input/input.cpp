#include "input/input.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

namespace vshade::input {
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

[[nodiscard]] bool isValid(const KeyCode key) {
    return key != KeyCode::Unknown && index(key) < key_count;
}

[[nodiscard]] bool isValid(const MouseButton button) {
    return index(button) < mouse_button_count;
}

} // namespace

bool Input::isKeyDown(const KeyCode key) {
    return isValid(key) && keys_down[index(key)];
}

bool Input::isKeyPressed(const KeyCode key) {
    return isValid(key) && keys_pressed[index(key)];
}

bool Input::isKeyReleased(const KeyCode key) {
    return isValid(key) && keys_released[index(key)];
}

bool Input::isMouseButtonDown(const MouseButton button) {
    return isValid(button) && mouse_buttons_down[index(button)];
}

bool Input::isMouseButtonPressed(const MouseButton button) {
    return isValid(button) && mouse_buttons_pressed[index(button)];
}

bool Input::isMouseButtonReleased(const MouseButton button) {
    return isValid(button) && mouse_buttons_released[index(button)];
}

glm::vec2 Input::mousePosition() {
    return current_mouse_position;
}

glm::vec2 Input::mouseDelta() {
    return current_mouse_delta;
}

glm::vec2 Input::scrollDelta() {
    return current_scroll_delta;
}

void Input::beginFrame() {
    std::fill(keys_pressed.begin(), keys_pressed.end(), false);
    std::fill(keys_released.begin(), keys_released.end(), false);
    std::fill(mouse_buttons_pressed.begin(), mouse_buttons_pressed.end(), false);
    std::fill(mouse_buttons_released.begin(), mouse_buttons_released.end(), false);

    current_mouse_delta = {};
    current_scroll_delta = {};
}

void Input::onKeyPressed(const KeyCode key) {
    if (!isValid(key)) {
        return;
    }

    const auto key_index = index(key);
    keys_pressed[key_index] = !keys_down[key_index];
    keys_down[key_index] = true;
}

void Input::onKeyReleased(const KeyCode key) {
    if (!isValid(key)) {
        return;
    }

    const auto key_index = index(key);
    keys_released[key_index] = keys_down[key_index];
    keys_down[key_index] = false;
}

void Input::onMouseButtonPressed(const MouseButton button) {
    if (!isValid(button)) {
        return;
    }

    const auto button_index = index(button);
    mouse_buttons_pressed[button_index] = !mouse_buttons_down[button_index];
    mouse_buttons_down[button_index] = true;
}

void Input::onMouseButtonReleased(const MouseButton button) {
    if (!isValid(button)) {
        return;
    }

    const auto button_index = index(button);
    mouse_buttons_released[button_index] = mouse_buttons_down[button_index];
    mouse_buttons_down[button_index] = false;
}

void Input::onMouseMoved(const float x, const float y) {
    const glm::vec2 new_position{x, y};
    if (has_mouse_position) {
        current_mouse_delta += new_position - current_mouse_position;
    }

    current_mouse_position = new_position;
    has_mouse_position = true;
}

void Input::onMouseScrolled(const float x, const float y) {
    current_scroll_delta += glm::vec2{x, y};
}

void Input::reset() {
    std::fill(keys_down.begin(), keys_down.end(), false);
    std::fill(keys_pressed.begin(), keys_pressed.end(), false);
    std::fill(keys_released.begin(), keys_released.end(), false);
    std::fill(mouse_buttons_down.begin(), mouse_buttons_down.end(), false);
    std::fill(mouse_buttons_pressed.begin(), mouse_buttons_pressed.end(), false);
    std::fill(mouse_buttons_released.begin(), mouse_buttons_released.end(), false);
    current_mouse_position = {};
    current_mouse_delta = {};
    current_scroll_delta = {};
    has_mouse_position = false;
}

void Input::resetMouseTracking() {
    current_mouse_delta = {};
    has_mouse_position = false;
}

} // namespace vshade::input
