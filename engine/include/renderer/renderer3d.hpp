#pragma once

#include "math/transform.hpp"
#include "math/vector.hpp"
#include "renderer/camera.hpp"
#include "renderer/material.hpp"
#include "renderer/mesh.hpp"
#include "renderer/model.hpp"

#include <cstdint>
#include <string_view>

namespace vshade::renderer {

/** @brief Directional light used by the initial lit material. */
struct DirectionalLight {
    /** @brief World-space direction in which the light rays travel. */
    math::Vec3 direction{-0.5F, -1.0F, -0.25F};
    /** @brief Red, green, and blue light color. */
    math::Vec3 color{1.0F};
    /** @brief Scalar brightness multiplier. */
    float intensity = 1.0F;
};

/** @brief Statistics collected by the basic 3D renderer. */
struct Renderer3DStats {
    /** @brief Number of GPU draw calls issued for the completed scene. */
    std::uint64_t drawCalls = 0;
    /** @brief Number of meshes submitted for the completed scene. */
    std::uint64_t meshCount = 0;
    /** @brief Number of persistent resource-set creations in this renderer lifetime. */
    std::uint64_t resourceInitializations = 0;
};

/**
 * @brief Required names and vertex locations for Renderer3D shader overrides.
 *
 * Position, normal, and texture-coordinate inputs use locations 0, 1, and 2.
 * The three matrix uniforms are the renderer-owned contract. Material and
 * per-draw parameters provide all other user-defined uniforms.
 */
struct Renderer3DShaderInterface {
    static constexpr std::uint32_t positionLocation = 0;
    static constexpr std::uint32_t normalLocation = 1;
    static constexpr std::uint32_t textureCoordinateLocation = 2;
    static constexpr std::string_view model = "model";
    static constexpr std::string_view view = "view";
    static constexpr std::string_view projection = "projection";
};

/**
 * @brief High-level 3D API for meshes, materials, cameras, and directional light.
 *
 * Mesh geometry is supplied by the caller. The renderer provides depth
 * testing, back-face culling, unlit materials, and basic lit materials. PBR,
 * tangents, and normal mapping are intentionally deferred.
 *
 */
class Renderer3D final {
public:
    Renderer3D() = delete;

    /**
     * @brief Starts a 3D scene using a perspective camera.
     * Configure @p camera with Camera::setPerspective() before beginning the
     * scene.
     *
     * @param camera Camera used to transform submitted meshes.
     * @throws std::logic_error If the low-level renderer is not initialized or
     * another 3D scene is already active.
     */
    static void beginScene(const Camera& camera);

    /**
     * @brief Sets the directional light used by basic lit materials.
     * @param light Direction, color, and intensity of the scene light.
     * @throws std::invalid_argument If the direction is zero or intensity is
     * negative or not finite.
     */
    static void setDirectionalLight(const DirectionalLight& light);

    /**
     * @brief Queues a mesh with a model transform and material.
     * @param transform Local-to-world transformation for the mesh.
     * @param mesh Geometry submitted for drawing.
     * @param material Shader, texture, color, and shared material parameters.
     * @param parameters Optional per-object values that override material values.
     * @throws std::logic_error If no 3D scene is active.
     */
    static void drawMesh(
        const math::Transform& transform,
        const Mesh& mesh,
        const Material& material,
        const DrawParameters& parameters = {}
    );

    /**
     * @brief Queues every primitive in a model's selected node hierarchy.
     * @param transform Local-to-world transformation applied above the model roots.
     * @param model Model containing mesh, material, and local node transforms.
     * @param parameters Optional per-object values applied to every model primitive.
     * @throws std::logic_error If no 3D scene is active.
     */
    static void drawModel(
        const math::Transform& transform,
        const Model& model,
        const DrawParameters& parameters = {}
    );

    /**
     * @brief Submits queued meshes and completes the active 3D scene.
     * @throws std::logic_error If no 3D scene is active.
     */
    static void endScene();

    /** @brief Returns statistics from the most recently completed 3D scene. */
    [[nodiscard]] static const Renderer3DStats& stats() noexcept;

private:
    friend class Renderer;

    /** Releases persistent renderer resources while the GL context is active. */
    static void shutdown() noexcept;
};

} // namespace vshade::renderer
