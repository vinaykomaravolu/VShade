#pragma once

#include "math/vector.hpp"
#include "renderer/rendertypes.hpp"

#include <cstddef>
#include <cstdint>

namespace vshade::renderer {

class Mesh;
class VertexArray;

/** @brief Statistics collected from renderer draw calls during the current frame. */
struct RenderStats {
    std::uint64_t drawCalls = 0;
    std::uint64_t indexCount = 0;
};

/** @brief Small OpenGL renderer facade used by the engine and games. */
class Renderer final {
public:
    Renderer() = delete;

    /** @brief Loads OpenGL functions and configures default renderer state. */
    static void initialize();

    /** @brief Releases renderer-global state. */
    static void shutdown() noexcept;

    /** @brief Returns whether initialize() completed successfully. */
    [[nodiscard]] static bool isInitialized() noexcept;

    /** @brief Resets per-frame statistics. */
    static void beginFrame() noexcept;

    /** @brief Sets the viewport in framebuffer pixels. */
    static void setViewport(std::uint32_t width, std::uint32_t height);

    /** @brief Sets the color used by clear(). */
    static void setClearColor(const math::Vec4& color);

    /** @brief Clears the active color and depth attachments. */
    static void clear();

    /** @brief Enables or disables depth testing. */
    static void setDepthTesting(bool enabled);

    /** @brief Enables or disables OpenGL color dithering. */
    static void setDithering(bool enabled);

    /** @brief Draws indexed geometry from @p vertexArray. */
    static void drawIndexed(
        const VertexArray& vertexArray,
        PrimitiveTopology topology = PrimitiveTopology::Triangles,
        std::size_t indexCount = 0
    );

    /** @brief Draws a Mesh using its configured topology. */
    static void draw(const Mesh& mesh);

    /** @brief Returns statistics accumulated since beginFrame(). */
    [[nodiscard]] static const RenderStats& stats() noexcept;
};

} // namespace vshade::renderer
