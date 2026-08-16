#include "renderer/Lighting.hpp"

#include "math/Vector.hpp"

#include <cmath>
#include <stdexcept>

namespace vshade::renderer {
namespace {

void validateColor(const math::Vec3& color) {
    if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
        !std::isfinite(color.b) || color.r < 0.0F || color.g < 0.0F ||
        color.b < 0.0F) {
        throw std::invalid_argument("Light color must be finite and non-negative");
    }
}

void validateIntensity(const float intensity) {
    if (!std::isfinite(intensity) || intensity < 0.0F) {
        throw std::invalid_argument("Light intensity must be finite and non-negative");
    }
}

} // namespace

Lighting::Lighting()
    : m_ambientLight(AmbientLight{.intensity = 0.15F}) {}

const std::optional<AmbientLight>& Lighting::ambientLight() const noexcept {
    return m_ambientLight;
}

void Lighting::setAmbientLight(const AmbientLight& light) {
    validateColor(light.color);
    validateIntensity(light.intensity);
    m_ambientLight = light;
}

void Lighting::clearAmbientLight() noexcept {
    m_ambientLight.reset();
}

const std::vector<DirectionalLight>& Lighting::directionalLights() const noexcept {
    return m_directionalLights;
}

void Lighting::addDirectionalLight(const DirectionalLight& light) {
    if (!std::isfinite(light.direction.x) || !std::isfinite(light.direction.y) ||
        !std::isfinite(light.direction.z) ||
        math::lengthSquared(light.direction) <= 0.000001F) {
        throw std::invalid_argument("Directional light direction must not be zero");
    }
    validateColor(light.color);
    validateIntensity(light.intensity);
    if (m_directionalLights.size() >= maximumDirectionalLights) {
        throw std::length_error("Too many directional lights");
    }
    DirectionalLight normalized = light;
    normalized.direction = math::normalize(light.direction);
    m_directionalLights.push_back(normalized);
}

void Lighting::clearDirectionalLights() noexcept {
    m_directionalLights.clear();
}

const std::vector<PointLight>& Lighting::pointLights() const noexcept {
    return m_pointLights;
}

void Lighting::addPointLight(const PointLight& light) {
    if (!std::isfinite(light.position.x) || !std::isfinite(light.position.y) ||
        !std::isfinite(light.position.z)) {
        throw std::invalid_argument("Point light position must be finite");
    }
    validateColor(light.color);
    validateIntensity(light.intensity);
    if (!std::isfinite(light.range) || light.range <= 0.0F) {
        throw std::invalid_argument("Point light range must be finite and positive");
    }
    if (m_pointLights.size() >= maximumPointLights) {
        throw std::length_error("Too many point lights");
    }
    m_pointLights.push_back(light);
}

void Lighting::clearPointLights() noexcept {
    m_pointLights.clear();
}

void Lighting::clear() noexcept {
    clearAmbientLight();
    clearDirectionalLights();
    clearPointLights();
}

} // namespace vshade::renderer
