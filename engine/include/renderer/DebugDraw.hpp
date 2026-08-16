#pragma once

#include "math/Vector.hpp"

#include <cstddef>
#include <cstdint>

namespace vshade::renderer {

class Camera;
class Renderer;

/** @brief World-space axis-aligned bounds used by debug wireframe boxes. */
struct DebugBounds {
    math::Vec3 minimum{0.0F};
    math::Vec3 maximum{0.0F};
};

/**
 * @brief Queues transient world-space primitives for debug visualization.
 *
 * Submitted primitives live for one flush. Boxes and spheres are represented
 * as lines, allowing colliders, bounds, rays, frustums, and light directions
 * to share the same eventual rendering path.
 */
class DebugDraw final {
public:
    DebugDraw() = delete;

    /** @brief Queues one colored world-space line segment. */
    static void line(
        const math::Vec3& start,
        const math::Vec3& end,
        const math::Vec4& color
    );

    /** @brief Queues the twelve edges of an axis-aligned wireframe box. */
    static void box(const DebugBounds& bounds, const math::Vec4& color);

    /**
     * @brief Queues three great-circle rings approximating a wireframe sphere.
     * @param segments Number of line segments in each ring.
     */
    static void sphere(
        const math::Vec3& center,
        float radius,
        const math::Vec4& color,
        std::uint32_t segments = 24
    );

    /**
     * @brief Renders all queued primitives with @p camera and clears the queue.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void flush(const Camera& camera);

    /** @brief Discards all queued primitives without rendering them. */
    static void clear() noexcept;

    /** @brief Returns the number of line segments currently queued. */
    [[nodiscard]] static std::size_t lineCount() noexcept;

private:
    friend class Renderer;

    /** Releases persistent GPU resources while the graphics context is active. */
    static void shutdown() noexcept;
};

} // namespace vshade::renderer
