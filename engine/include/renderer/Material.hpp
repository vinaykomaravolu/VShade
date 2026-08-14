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

/** @brief Controls how a material's alpha channel affects rasterization. */
enum class MaterialAlphaMode {
    /** @brief Ignores alpha for coverage and writes every fragment. */
    Opaque,
    /** @brief Discards fragments below the configured alpha cutoff. */
    Mask,
    /** @brief Blends fragments with the color already in the framebuffer. */
    Blend,
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

    /** @brief Returns the optional tangent-space normal texture. */
    [[nodiscard]] const std::shared_ptr<Texture2D>& normalTexture() const noexcept;

    /** @brief Replaces or clears the tangent-space normal texture. */
    void setNormalTexture(std::shared_ptr<Texture2D> texture) noexcept;

    /** @brief Reports whether a normal texture is assigned. */
    [[nodiscard]] bool hasNormalTexture() const noexcept;

    /** @brief Returns the multiplier applied to normal-map X and Y channels. */
    [[nodiscard]] float normalScale() const noexcept;

    /**
     * @brief Replaces the tangent-space normal strength.
     * @throws std::invalid_argument If @p scale is negative or not finite.
     */
    void setNormalScale(float scale);

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

    /** @brief Returns how the material handles fragment alpha. */
    [[nodiscard]] MaterialAlphaMode alphaMode() const noexcept;

    /** @brief Replaces the fragment-alpha behavior. */
    void setAlphaMode(MaterialAlphaMode mode) noexcept;

    /** @brief Returns the alpha threshold used by masked materials. */
    [[nodiscard]] float alphaCutoff() const noexcept;

    /**
     * @brief Replaces the masked-material alpha threshold.
     * @throws std::invalid_argument If @p cutoff is outside zero through one.
     */
    void setAlphaCutoff(float cutoff);

    /** @brief Reports whether both sides of triangles should be rasterized. */
    [[nodiscard]] bool doubleSided() const noexcept;

    /** @brief Enables or disables rendering both sides of triangles. */
    void setDoubleSided(bool doubleSided) noexcept;

    /** @brief Returns custom shader values shared by every draw using this material. */
    [[nodiscard]] const MaterialParameters& parameters() const noexcept;

    /** @brief Returns mutable custom shader values for this material. */
    [[nodiscard]] MaterialParameters& parameters() noexcept;

private:
    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Texture2D> m_albedoTexture;
    std::shared_ptr<Texture2D> m_normalTexture;
    math::Vec4 m_albedoColor{1.0F};
    float m_roughness = 0.5F;
    float m_metallic = 0.0F;
    float m_normalScale = 1.0F;
    float m_alphaCutoff = 0.5F;
    MaterialShading m_shading = MaterialShading::Unlit;
    MaterialAlphaMode m_alphaMode = MaterialAlphaMode::Opaque;
    bool m_doubleSided = false;
    MaterialParameters m_parameters;
};

} // namespace vshade::renderer
