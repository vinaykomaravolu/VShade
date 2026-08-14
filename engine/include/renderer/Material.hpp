#pragma once

#include "math/Vector.hpp"
#include "renderer/MaterialParameters.hpp"
#include "renderer/Shader.hpp"
#include "renderer/Texture.hpp"

#include <memory>

namespace vshade::renderer {

/** @brief Shading model used by a basic material. */
enum class MaterialShading {
    /** @brief Displays albedo without applying scene lighting. */
    Unlit,
    /** @brief Applies the renderer's basic directional lighting. */
    Lit,
};

/** @brief Owns the resources and surface properties used to draw a mesh. */
class Material final {
public:
    /** @brief Creates a white, untextured, unlit material. */
    Material() = default;

    /**
     * @brief Creates a material using the selected shading model.
     * @param shading Initial unlit or basic-lit shading model.
     */
    explicit Material(MaterialShading shading) noexcept;

    /**
     * @brief Returns the optional shader override.
     * @return Shared shader, or null when the renderer should use its default.
     */
    [[nodiscard]] const std::shared_ptr<Shader>& shader() const noexcept;

    /**
     * @brief Replaces or clears the shader override.
     * @param shader Shared shader, or null to use the renderer default.
     */
    void setShader(std::shared_ptr<Shader> shader) noexcept;

    /**
     * @brief Reports whether a shader override is assigned.
     * @return True when shader() is non-null.
     */
    [[nodiscard]] bool hasShader() const noexcept;

    /**
     * @brief Returns the optional albedo texture.
     * @return Shared albedo texture, or null for a solid-color material.
     */
    [[nodiscard]] const std::shared_ptr<Texture2D>& albedoTexture() const noexcept;

    /**
     * @brief Replaces or clears the albedo texture.
     * @param texture Shared texture, or null for a solid-color material.
     */
    void setAlbedoTexture(std::shared_ptr<Texture2D> texture) noexcept;

    /**
     * @brief Reports whether an albedo texture is assigned.
     * @return True when albedoTexture() is non-null.
     */
    [[nodiscard]] bool hasAlbedoTexture() const noexcept;

    /**
     * @brief Returns the color multiplied with the albedo texture.
     * @return Red, green, blue, and alpha albedo multiplier.
     */
    [[nodiscard]] const math::Vec4& albedoColor() const noexcept;

    /**
     * @brief Replaces the albedo color multiplier.
     * @param color Red, green, blue, and alpha multiplier.
     */
    void setAlbedoColor(const math::Vec4& color) noexcept;

    /**
     * @brief Returns the surface roughness.
     * @return Roughness in the inclusive range zero through one.
     */
    [[nodiscard]] float roughness() const noexcept;

    /**
     * @brief Replaces the surface roughness.
     * @param roughness Roughness in the inclusive range zero through one.
     * @throws std::invalid_argument If @p roughness is outside the valid range or not finite.
     */
    void setRoughness(float roughness);

    /**
     * @brief Returns the metallic surface amount.
     * @return Metallic value in the inclusive range zero through one.
     */
    [[nodiscard]] float metallic() const noexcept;

    /**
     * @brief Replaces the metallic surface amount.
     * @param metallic Metallic value in the inclusive range zero through one.
     * @throws std::invalid_argument If @p metallic is outside the valid range or not finite.
     */
    void setMetallic(float metallic);

    /**
     * @brief Returns the active shading model.
     * @return Unlit or basic-lit shading selection.
     */
    [[nodiscard]] MaterialShading shading() const noexcept;

    /**
     * @brief Replaces the shading model.
     * @param shading New unlit or basic-lit shading model.
     */
    void setShading(MaterialShading shading) noexcept;

    /** @brief Returns custom shader values shared by every draw using this material. */
    [[nodiscard]] const MaterialParameters& parameters() const noexcept;

    /** @brief Returns mutable custom shader values for this material. */
    [[nodiscard]] MaterialParameters& parameters() noexcept;

private:
    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Texture2D> m_albedoTexture;
    math::Vec4 m_albedoColor{1.0F};
    float m_roughness = 0.5F;
    float m_metallic = 0.0F;
    MaterialShading m_shading = MaterialShading::Unlit;
    MaterialParameters m_parameters;
};

} // namespace vshade::renderer
