#include "visual/imagecomparison.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/vector.hpp>
#include <platform/window.hpp>
#include <renderer/buffer.hpp>
#include <renderer/framebuffer.hpp>
#include <renderer/renderer.hpp>
#include <renderer/shader.hpp>
#include <renderer/vertexarray.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace {

constexpr std::uint32_t renderWidth = 512;
constexpr std::uint32_t renderHeight = 512;

class HiddenRenderContext final {
public:
    HiddenRenderContext()
        : m_window({
              .title = "VShade visual test",
              .width = renderWidth,
              .height = renderHeight,
              .fullscreen = false,
              .vsync = false,
              .visible = false,
          }) {
        vshade::renderer::Renderer::initialize();
    }

    ~HiddenRenderContext() {
        vshade::renderer::Renderer::shutdown();
    }

    HiddenRenderContext(const HiddenRenderContext&) = delete;
    HiddenRenderContext& operator=(const HiddenRenderContext&) = delete;

private:
    vshade::platform::Window m_window;
};

struct TriangleVertex {
    vshade::math::Vec3 position;
};

} // namespace

TEST_CASE("Offscreen colored triangle matches its golden image", "[renderer][visual]") {
    HiddenRenderContext context;

    constexpr std::array<TriangleVertex, 3> vertices{{
        {{-0.65F, -0.55F, 0.0F}},
        {{0.65F, -0.55F, 0.0F}},
        {{0.0F, 0.65F, 0.0F}},
    }};
    constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
    });

    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    auto vertexArray = std::make_shared<vshade::renderer::VertexArray>();
    vertexArray->addVertexBuffer(vertexBuffer);
    vertexArray->setIndexBuffer(indexBuffer);

    const vshade::renderer::Shader shader(
        "visual-test-triangle",
        R"glsl(#version 330 core
layout(location = 0) in vec3 position;

void main() {
    gl_Position = vec4(position, 1.0);
}
)glsl",
        R"glsl(#version 330 core
out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(0.90, 0.25, 0.10, 1.0);
}
)glsl"
    );

    vshade::renderer::Framebuffer framebuffer(renderWidth, renderHeight);
    framebuffer.bind();
    vshade::renderer::Renderer::beginFrame();
    vshade::renderer::Renderer::setDepthTesting(false);
    vshade::renderer::Renderer::setDithering(false);
    vshade::renderer::Renderer::setClearColor({0.04F, 0.06F, 0.10F, 1.0F});
    vshade::renderer::Renderer::clear();
    shader.bind();
    vshade::renderer::Renderer::drawIndexed(*vertexArray);

    const vshade::tests::visual::Image actual{
        renderWidth,
        renderHeight,
        framebuffer.readPixels(),
    };
    vshade::renderer::Framebuffer::unbind();

    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().indexCount == 3);

    const std::filesystem::path goldenPath =
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "colored_triangle.png";
    const std::filesystem::path outputDirectory =
        std::filesystem::path(VSHADE_VISUAL_OUTPUT_DIR) / "colored_triangle";

    if (!std::filesystem::exists(goldenPath)) {
        vshade::tests::visual::writePng(outputDirectory / "actual.png", actual);
        FAIL(
            "Missing golden image: " << goldenPath.string()
            << ". The rendered candidate was written to "
            << (outputDirectory / "actual.png").string()
            << ". Review it and add it manually; the test will not overwrite goldens."
        );
    }

    const vshade::tests::visual::ComparisonTolerance tolerance{
        .channelError = 4,
        .meanError = 1.0,
        .maximumError = 255,
        .pixelsOutsidePercentage = 1.0,
    };
    const auto result = vshade::tests::visual::compareAgainstGolden(
        actual,
        goldenPath,
        outputDirectory,
        tolerance
    );

    INFO(result.summary());
    INFO("Failure artifacts: " << outputDirectory.string());
    CHECK(result.passed);
}

TEST_CASE("Image comparison reports errors and writes failure artifacts", "[renderer][visual]") {
    const std::filesystem::path outputDirectory =
        std::filesystem::path(VSHADE_VISUAL_OUTPUT_DIR) / "comparison_utility";
    const std::filesystem::path temporaryGolden = outputDirectory / "golden.png";
    std::filesystem::remove_all(outputDirectory);

    const vshade::tests::visual::Image expected{
        2,
        1,
        {
            10, 20, 30, 255,
            40, 50, 60, 255,
        },
    };
    const vshade::tests::visual::Image actual{
        2,
        1,
        {
            20, 20, 30, 255,
            40, 50, 60, 255,
        },
    };
    vshade::tests::visual::writePng(temporaryGolden, expected);

    const vshade::tests::visual::ComparisonTolerance tolerance{
        .channelError = 4,
        .meanError = 0.0,
        .maximumError = 255,
        .pixelsOutsidePercentage = 0.0,
    };
    const auto result = vshade::tests::visual::compareAgainstGolden(
        actual,
        temporaryGolden,
        outputDirectory,
        tolerance
    );

    CHECK_FALSE(result.passed);
    CHECK(result.dimensionsMatch);
    CHECK(result.meanPixelError == 1.25);
    CHECK(result.maximumPixelError == 10);
    CHECK(result.pixelsOutsideTolerance == 1);
    CHECK(result.totalPixels == 2);
    CHECK(result.pixelsOutsidePercentage == 50.0);
    CHECK(std::filesystem::exists(outputDirectory / "actual.png"));
    CHECK(std::filesystem::exists(outputDirectory / "expected.png"));
    CHECK(std::filesystem::exists(outputDirectory / "difference.png"));

    std::filesystem::remove_all(outputDirectory);
}
