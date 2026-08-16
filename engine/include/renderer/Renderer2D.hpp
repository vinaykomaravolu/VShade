#pragma once

#include "math/Transform.hpp"
#include "math/Vector.hpp"
#include "renderer/Camera.hpp"
#include "renderer/Texture.hpp"

#include <cstdint>
#include <memory>

namespace vshade::renderer {

/** @brief Data required to render one sprite. */
struct SpriteRendererComponent {
    /** @brief Optional texture; null produces a color-only sprite. */
    std::shared_ptr<Texture2D> texture;
    /** @brief Texture tint or solid sprite color. */
    math::Vec4 color{1.0F};
    /** @brief Number of texture repetitions across the sprite. */
    math::Vec2 tiling{1.0F};
    /** @brief Lower layers are submitted before higher layers. */
    std::int32_t sortingLayer = 0;
};

/** @brief Statistics collected by the 2D renderer. */
struct Renderer2DStats {
    /** @brief Number of GPU draw calls issued for the completed scene. */
    std::uint64_t drawCalls = 0;
    /** @brief Number of quads submitted for the completed scene. */
    std::uint64_t quadCount = 0;
    /** @brief Number of persistent resource-set creations in this renderer lifetime. */
    std::uint64_t resourceInitializations = 0;
};

/**
 * @brief High-level 2D drawing API for colored quads, textures, and sprites.
 *
 * Draw calls are queued between beginScene() and endScene(). The first
 * implementation should prioritize correctness and sorting by layer; batching
 * many sprites into shared buffers can be added afterward.
 *
 */
class Renderer2D final {
public:
    Renderer2D() = delete;

    /**
     * @brief Starts a 2D scene using an orthographic camera.
     * Configure @p camera with Camera::setOrthographic() before beginning the
     * scene. Pixel-space bounds can span zero through the framebuffer width
     * and height, while world-space bounds can use values such as -10 to 10.
     *
     * @param camera Camera used to transform submitted geometry.
     * @throws std::logic_error If the low-level renderer is not initialized or
     * another 2D scene is already active.
     */
    static void beginScene(const Camera& camera);

    /**
     * @brief Queues a solid-color quad.
     * @param transform Position, rotation, and scale of the quad.
     * @param color Red, green, blue, and alpha color.
     * @param sortingLayer Layer used to order transparent 2D geometry.
     * @throws std::logic_error If no 2D scene is active.
     */
    static void drawQuad(
        const math::Transform& transform,
        const math::Vec4& color,
        std::int32_t sortingLayer = 0
    );

    /**
     * @brief Queues a textured quad.
     * @param transform Position, rotation, and scale of the quad.
     * @param texture Texture sampled by the quad.
     * @param tint Color multiplied with the sampled texture.
     * @param tiling Number of texture repetitions across the quad.
     * @param sortingLayer Layer used to order transparent 2D geometry.
     * @throws std::logic_error If no 2D scene is active.
     * @warning @p texture must remain alive until endScene() returns.
     */
    static void drawQuad(
        const math::Transform& transform,
        const Texture2D& texture,
        const math::Vec4& tint = math::Vec4{1.0F},
        const math::Vec2& tiling = math::Vec2{1.0F},
        std::int32_t sortingLayer = 0
    );

    /**
     * @brief Queues a sprite component.
     * @param transform Position, rotation, and scale of the sprite.
     * @param sprite Texture, tint, tiling, and sorting data.
     * @throws std::logic_error If no 2D scene is active.
     */
    static void drawSprite(
        const math::Transform& transform,
        const SpriteRendererComponent& sprite
    );

    /**
     * @brief Sorts, submits, and completes the active 2D scene.
     * @throws std::logic_error If no 2D scene is active.
     */
    static void endScene();

    /** @brief Returns statistics from the most recently completed 2D scene. */
    [[nodiscard]] static const Renderer2DStats& stats() noexcept;

private:
    friend class Renderer;

    /** Releases persistent renderer resources while the GL context is active. */
    static void shutdown() noexcept;
};

} // namespace vshade::renderer
