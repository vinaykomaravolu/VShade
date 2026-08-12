#include "renderer/material.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace vshade::renderer {
namespace {

void validateUnitRange(const float value, const char* propertyName) {
    if (!std::isfinite(value) || value < 0.0F || value > 1.0F) {
        throw std::invalid_argument(
            std::string(propertyName) + " must be finite and between zero and one"
        );
    }
}

} // namespace

Material::Material(const MaterialShading shading) noexcept
    : m_shading(shading) {}

const std::shared_ptr<Shader>& Material::shader() const noexcept {
    return m_shader;
}

void Material::setShader(std::shared_ptr<Shader> shader) noexcept {
    m_shader = std::move(shader);
}

bool Material::hasShader() const noexcept {
    return m_shader != nullptr;
}

const std::shared_ptr<Texture2D>& Material::albedoTexture() const noexcept {
    return m_albedoTexture;
}

void Material::setAlbedoTexture(std::shared_ptr<Texture2D> texture) noexcept {
    m_albedoTexture = std::move(texture);
}

bool Material::hasAlbedoTexture() const noexcept {
    return m_albedoTexture != nullptr;
}

const math::Vec4& Material::albedoColor() const noexcept {
    return m_albedoColor;
}

void Material::setAlbedoColor(const math::Vec4& color) noexcept {
    m_albedoColor = color;
}

float Material::roughness() const noexcept {
    return m_roughness;
}

void Material::setRoughness(const float roughness) {
    validateUnitRange(roughness, "Material roughness");
    m_roughness = roughness;
}

float Material::metallic() const noexcept {
    return m_metallic;
}

void Material::setMetallic(const float metallic) {
    validateUnitRange(metallic, "Material metallic value");
    m_metallic = metallic;
}

MaterialShading Material::shading() const noexcept {
    return m_shading;
}

void Material::setShading(const MaterialShading shading) noexcept {
    m_shading = shading;
}

} // namespace vshade::renderer
