#pragma once

#include "math/Vector.hpp"
#include "renderer/RenderTypes.hpp"

#include <cstddef>
#include <cstdint>

namespace vshade::renderer {

class Mesh;
class Texture2D;
class VertexArray;
class Renderer;

/** @brief Statistics collected from renderer draw calls during the current frame. */
struct RenderStats {
    std::uint64_t drawCalls = 0;
    std::uint64_t indexCount = 0;
    std::uint64_t vertexCount = 0;
};

/** @brief Complete pipeline state managed by the renderer facade. */
struct PipelineState {
    bool blending = true;
    BlendFactor sourceBlend = BlendFactor::SourceAlpha;
    BlendFactor destinationBlend = BlendFactor::OneMinusSourceAlpha;
    bool faceCulling = false;
    CullFace cullFace = CullFace::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    PolygonMode polygonMode = PolygonMode::Fill;
    bool depthTesting = true;
    DepthFunction depthFunction = DepthFunction::Less;
    bool depthWrite = true;
    bool dithering = true;

    bool operator==(const PipelineState&) const = default;
};

/**
 * @brief Restores a captured renderer pipeline state when it leaves scope.
 *
 * Guards are movable but not copyable. A moved-from guard becomes inactive,
 * ensuring that each captured state is restored exactly once.
 */
class PipelineStateGuard final {
public:
    ~PipelineStateGuard() noexcept;

    PipelineStateGuard(const PipelineStateGuard&) = delete;
    PipelineStateGuard& operator=(const PipelineStateGuard&) = delete;
    PipelineStateGuard(PipelineStateGuard&& other) noexcept;
    PipelineStateGuard& operator=(PipelineStateGuard&& other) noexcept;

    /** @brief Restores the captured state immediately and deactivates this guard. */
    void restore() noexcept;

    /** @brief Reports whether this guard still owns a pending restoration. */
    [[nodiscard]] bool active() const noexcept;

private:
    friend class Renderer;

    explicit PipelineStateGuard(const PipelineState& state) noexcept;

    PipelineState m_state{};
    bool m_active = true;
};

/** @brief Viewport rectangle tracked by the renderer facade. */
struct Viewport {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    bool operator==(const Viewport&) const = default;
};

/**
 * @brief Small OpenGL renderer facade used by the engine and games.
 * @note Except for lifecycle queries, call initialize() before using renderer operations.
 */
class Renderer final {
public:
    Renderer() = delete;

    /**
     * @brief Loads OpenGL functions and configures default renderer state.
     * @throws std::logic_error If no OpenGL context is current.
     * @throws std::runtime_error If OpenGL functions cannot be loaded.
     */
    static void initialize();

    /** @brief Releases renderer-global state. */
    static void shutdown() noexcept;

    /**
     * @brief Returns whether initialize() completed successfully.
     * @return True while the renderer is initialized; otherwise false.
     */
    [[nodiscard]] static bool isInitialized() noexcept;

    /** @brief Resets per-frame statistics. */
    static void beginFrame() noexcept;

    /**
     * @brief Validates that the frame produced no OpenGL errors in debug builds.
     * @throws std::runtime_error If OpenGL reports an error in a debug build.
     */
    static void endFrame();

    /** @brief Returns the pipeline state currently tracked by Renderer. */
    [[nodiscard]] static PipelineState pipelineState() noexcept;

    /**
     * @brief Captures the current pipeline state in a scoped restoration guard.
     * @return Movable guard that restores the captured state when destroyed.
     */
    [[nodiscard]] static PipelineStateGuard pushPipelineState() noexcept;

    /** @brief Applies a complete pipeline state through the renderer facade. */
    static void applyPipelineState(const PipelineState& state);

    /**
     * @brief Sets the viewport in framebuffer pixels.
     * @param x Horizontal offset from the framebuffer origin.
     * @param y Vertical offset from the framebuffer origin.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::overflow_error If a value cannot be represented by OpenGL.
     */
    static void setViewport(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height
    );

    /** @brief Returns the viewport most recently set through Renderer. */
    [[nodiscard]] static Viewport viewport() noexcept;

    /**
     * @brief Sets the color used by clear().
     * @param color Red, green, blue, and alpha clear values.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setClearColor(const math::Vec4& color);

    /**
     * @brief Clears the selected attachments of the active framebuffer.
     * @param flags Framebuffer attachments to clear.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p flags contains an unknown bit.
     */
    static void clear(ClearFlags flags = ClearFlags::Color | ClearFlags::Depth);

    /**
     * @brief Enables or disables blending.
     * @param enabled True to enable fragment blending.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setBlending(bool enabled);

    /**
     * @brief Sets the source and destination blend factors.
     * @param source Factor applied to the source fragment.
     * @param destination Factor applied to the framebuffer destination.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If a blend factor is not recognized.
     */
    static void setBlendFunction(BlendFactor source, BlendFactor destination);

    /**
     * @brief Enables or disables face culling.
     * @param enabled True to discard the configured face orientation.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setFaceCulling(bool enabled);

    /**
     * @brief Sets which faces are discarded when face culling is enabled.
     * @param face Face orientation to discard.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p face is not recognized.
     */
    static void setCullFace(CullFace face);

    /**
     * @brief Sets the winding order considered front-facing.
     * @param winding Vertex winding treated as the front face.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p winding is not recognized.
     */
    static void setFrontFace(FrontFace winding);

    /**
     * @brief Sets how polygon faces are rasterized.
     * @param mode Fill, line, or point rasterization mode.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p mode is not recognized.
     */
    static void setPolygonMode(PolygonMode mode);

    /**
     * @brief Enables or disables depth testing.
     * @param enabled True to compare fragment depth before drawing.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setDepthTesting(bool enabled);

    /**
     * @brief Sets the comparison used by depth testing.
     * @param function Comparison applied to incoming fragment depth.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p function is not recognized.
     */
    static void setDepthFunction(DepthFunction function);

    /**
     * @brief Enables or disables writing fragment depth values.
     * @param enabled True to allow writes to the depth attachment.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setDepthWrite(bool enabled);

    /**
     * @brief Enables or disables OpenGL color dithering.
     * @param enabled True to enable color dithering.
     * @throws std::logic_error If the renderer is not initialized.
     */
    static void setDithering(bool enabled);

    /**
     * @brief Draws indexed geometry from @p vertexArray.
     * @param vertexArray Vertex and index buffers to bind for the draw.
     * @param topology Primitive assembly mode.
     * @param indexCount Number of indices to draw, or zero to use the complete index buffer.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If @p vertexArray has no index buffer.
     * @throws std::out_of_range If @p indexCount exceeds the index buffer.
     * @throws std::overflow_error If the draw count cannot be represented by OpenGL.
     */
    static void drawIndexed(
        const VertexArray& vertexArray,
        PrimitiveTopology topology = PrimitiveTopology::Triangles,
        std::size_t indexCount = 0
    );

    /**
     * @brief Draws non-indexed geometry from @p vertexArray.
     * @param vertexArray Vertex buffers to bind for the draw.
     * @param topology Primitive assembly mode.
     * @param vertexCount Number of sequential vertices to draw.
     * @param firstVertex Index of the first vertex to draw.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If no vertex buffer exists or @p vertexCount is zero.
     * @throws std::out_of_range If the requested range exceeds a vertex buffer.
     * @throws std::overflow_error If the draw range cannot be represented by OpenGL.
     */
    static void drawArrays(
        const VertexArray& vertexArray,
        PrimitiveTopology topology,
        std::size_t vertexCount,
        std::size_t firstVertex = 0
    );

    /**
     * @brief Draws a Mesh using its configured topology.
     * @param mesh Mesh whose geometry should be submitted.
     * @throws std::logic_error If the renderer is not initialized.
     * @throws std::invalid_argument If the mesh has no index buffer.
     * @throws std::out_of_range If the configured draw range is invalid.
     * @throws std::overflow_error If the draw count cannot be represented by OpenGL.
     */
    static void draw(const Mesh& mesh);

    /**
     * @brief Returns statistics accumulated since beginFrame().
     * @return Read-only reference to the current frame statistics.
     */
    [[nodiscard]] static const RenderStats& stats() noexcept;

    /** @brief Returns the texture-unit limit cached at renderer initialization. */
    [[nodiscard]] static std::uint32_t maximumTextureSlots();
};

} // namespace vshade::renderer
