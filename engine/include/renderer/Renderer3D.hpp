#pragma once

#include "math/Transform.hpp"
#include "math/Vector.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Lighting.hpp"
#include "renderer/Material.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/Model.hpp"

#include <cstdint>
#include <string_view>

namespace vshade::renderer {

class Renderer3DSceneScope;

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
 * Position, normal, texture-coordinate, and tangent inputs use locations 0-3.
 * The three matrix uniforms are the renderer-owned contract. Material and
 * per-draw parameters provide all other user-defined uniforms.
 */
struct Renderer3DShaderInterface {
    static constexpr std::uint32_t positionLocation = 0;
    static constexpr std::uint32_t normalLocation = 1;
    static constexpr std::uint32_t textureCoordinateLocation = 2;
    static constexpr std::uint32_t tangentLocation = 3;
    static constexpr std::string_view model = "model";
    static constexpr std::string_view view = "view";
    static constexpr std::string_view projection = "projection";
};

/**
 * @brief High-level 3D API for meshes, materials, cameras, and directional light.
 *
 * Mesh geometry is supplied by the caller. The renderer provides depth
 * testing, material-controlled culling and alpha, unlit materials, basic lit
 * materials, and tangent-space normal maps.
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

    /** @brief Begins an exception-safe scene scope that ends on destruction. */
    [[nodiscard]] static Renderer3DSceneScope scopedScene(const Camera& camera);

    /**
     * @brief Sets the directional light used by basic lit materials.
     * @param light Direction, color, and intensity of the scene light.
     * @throws std::invalid_argument If the direction is zero or intensity is
     * negative or not finite.
     */
    static void setDirectionalLight(const DirectionalLight& light);

    /** @brief Replaces all ambient, directional, and point lights. */
    static void setLighting(const Lighting& lighting);

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
        const DrawParameters& parameters = {},
        std::int32_t entityId = -1
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
        const DrawParameters& parameters = {},
        std::int32_t entityId = -1
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

/** @brief Movable RAII adapter over Renderer3D begin/draw/end submission. */
class Renderer3DSceneScope final {
public:
    ~Renderer3DSceneScope() noexcept;
    Renderer3DSceneScope(Renderer3DSceneScope&& other) noexcept;
    Renderer3DSceneScope& operator=(Renderer3DSceneScope&& other) noexcept;
    Renderer3DSceneScope(const Renderer3DSceneScope&) = delete;
    Renderer3DSceneScope& operator=(const Renderer3DSceneScope&) = delete;

    void setLighting(const Lighting& lighting);
    void draw(
        const math::Transform& transform,
        const Model& model,
        const DrawParameters& parameters = {}
    );
    void draw(
        const math::Transform& transform,
        const Mesh& mesh,
        const Material& material,
        const DrawParameters& parameters = {}
    );
    void finish();

private:
    friend class Renderer3D;
    explicit Renderer3DSceneScope(const Camera& camera);
    bool m_active = true;
};

} // namespace vshade::renderer
