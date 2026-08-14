#include "renderer/MaterialParameters.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace vshade::renderer {

void MaterialParameters::setValue(
    const std::string_view name,
    MaterialParameterValue value
) {
    if (name.empty()) {
        throw std::invalid_argument("Material parameter name must not be empty");
    }
    m_values.insert_or_assign(std::string(name), std::move(value));
}

void MaterialParameters::set(const std::string_view name, const int value) {
    setValue(name, value);
}

void MaterialParameters::set(const std::string_view name, const float value) {
    setValue(name, value);
}

void MaterialParameters::set(const std::string_view name, const math::Vec2& value) {
    setValue(name, value);
}

void MaterialParameters::set(const std::string_view name, const math::Vec3& value) {
    setValue(name, value);
}

void MaterialParameters::set(const std::string_view name, const math::Vec4& value) {
    setValue(name, value);
}

void MaterialParameters::set(const std::string_view name, const math::Mat4& value) {
    setValue(name, value);
}

void MaterialParameters::set(
    const std::string_view name,
    std::shared_ptr<Texture2D> value
) {
    if (!value) {
        throw std::invalid_argument("Material texture parameter must not be null");
    }
    setValue(name, std::move(value));
}

bool MaterialParameters::erase(const std::string_view name) {
    return m_values.erase(std::string(name)) != 0;
}

void MaterialParameters::clear() noexcept {
    m_values.clear();
}

bool MaterialParameters::contains(const std::string_view name) const {
    return m_values.contains(std::string(name));
}

bool MaterialParameters::empty() const noexcept {
    return m_values.empty();
}

std::size_t MaterialParameters::size() const noexcept {
    return m_values.size();
}

const MaterialParameters::Values& MaterialParameters::values() const noexcept {
    return m_values;
}

} // namespace vshade::renderer
