#pragma once

#include "visual/imagecomparison.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/vector.hpp>
#include <platform/window.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace vshade::tests::visual {

constexpr std::uint32_t defaultRenderWidth = 512;
constexpr std::uint32_t defaultRenderHeight = 512;

class HiddenRenderContext final {
public:
    HiddenRenderContext(
        const std::uint32_t width = defaultRenderWidth,
        const std::uint32_t height = defaultRenderHeight
    ) : m_window({
            .title = "VShade renderer test",
            .width = width,
            .height = height,
            .fullscreen = false,
            .vsync = false,
            .visible = false,
        }) {
        renderer::Renderer::initialize();
    }

    ~HiddenRenderContext() {
        renderer::Renderer::shutdown();
    }

    HiddenRenderContext(const HiddenRenderContext&) = delete;
    HiddenRenderContext& operator=(const HiddenRenderContext&) = delete;

private:
    platform::Window m_window;
};

inline void beginOffscreenFrame(
    renderer::Framebuffer& framebuffer,
    const math::Vec4& clearColor
) {
    framebuffer.bind();
    renderer::Renderer::beginFrame();
    renderer::Renderer::setViewport(0, 0, framebuffer.width(), framebuffer.height());
    renderer::Renderer::setDithering(false);
    renderer::Renderer::setClearColor(clearColor);
    renderer::Renderer::clear(
        renderer::ClearFlags::Color |
        renderer::ClearFlags::Depth |
        renderer::ClearFlags::Stencil
    );
}

[[nodiscard]] inline Image captureFramebuffer(const renderer::Framebuffer& framebuffer) {
    Image image{
        framebuffer.width(),
        framebuffer.height(),
        framebuffer.readPixels(),
    };
    renderer::Framebuffer::unbind();
    return image;
}

inline void checkGoldenImage(const Image& actual, const std::string_view imageName) {
    const std::string filename(imageName);
    const std::filesystem::path goldenPath =
        std::filesystem::path(VSHADE_GOLDEN_DIR) / (filename + ".png");
    const std::filesystem::path outputDirectory =
        std::filesystem::path(VSHADE_VISUAL_OUTPUT_DIR) / filename;

    if (!std::filesystem::exists(goldenPath)) {
        writePng(outputDirectory / "actual.png", actual);
        FAIL(
            "Missing golden image: " << goldenPath.string()
            << ". Review the candidate at "
            << (outputDirectory / "actual.png").string()
        );
    }

    const ComparisonTolerance tolerance{
        .channelError = 4,
        .meanError = 1.0,
        .maximumError = 255,
        .pixelsOutsidePercentage = 1.0,
    };
    const ComparisonResult result = compareAgainstGolden(
        actual,
        goldenPath,
        outputDirectory,
        tolerance
    );

    INFO(result.summary());
    INFO("Failure artifacts: " << outputDirectory.string());
    CHECK(result.passed);
}

} // namespace vshade::tests::visual
