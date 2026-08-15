#pragma once

#include <cstdint>

namespace vshade::renderer {

/** @brief Statistics collected from renderer draw calls during the current frame. */
struct RenderStats {
    std::uint64_t drawCalls = 0;
    std::uint64_t indexCount = 0;
    std::uint64_t vertexCount = 0;
    std::uint64_t triangleCount = 0;
};

/**
 * @brief Snapshot consumed by a debug or editor performance overlay.
 *
 * Timing and entity values are supplied by the application or scene while
 * rendering values come from Renderer::stats(). Keeping the renderer values
 * nested preserves that ownership boundary.
 */
struct DebugFrameStats {
    float framesPerSecond = 0.0F;
    float frameTimeMilliseconds = 0.0F;
    RenderStats rendering{};
    std::uint64_t entityCount = 0;
    std::uint64_t modelDrawCalls = 0;
    std::uint64_t meshCount = 0;
    std::uint64_t spriteDrawCalls = 0;
    std::uint64_t quadCount = 0;
    std::uint64_t debugLineCount = 0;
};

} // namespace vshade::renderer
