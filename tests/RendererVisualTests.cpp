#include "visual/ImageComparison.hpp"

#include <catch2/catch_test_macros.hpp>

#include <math/Matrix.hpp>
#include <math/Quaternion.hpp>
#include <math/Vector.hpp>
#include <platform/Window.hpp>
#include <renderer/Buffer.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Shader.hpp>
#include <renderer/Texture.hpp>
#include <renderer/VertexArray.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

struct TexturedVertex {
    vshade::math::Vec3 position;
    vshade::math::Vec2 textureCoordinate;
};

struct ColoredVertex {
    vshade::math::Vec3 position;
    vshade::math::Vec3 color;
};

void beginOffscreenFrame(
    vshade::renderer::Framebuffer& framebuffer,
    const vshade::math::Vec4& clearColor
) {
    framebuffer.bind();
    vshade::renderer::Renderer::beginFrame();
    vshade::renderer::Renderer::setViewport(0, 0, renderWidth, renderHeight);
    vshade::renderer::Renderer::setBlending(false);
    vshade::renderer::Renderer::setBlendFunction(
        vshade::renderer::BlendFactor::SourceAlpha,
        vshade::renderer::BlendFactor::OneMinusSourceAlpha
    );
    vshade::renderer::Renderer::setFaceCulling(true);
    vshade::renderer::Renderer::setCullFace(vshade::renderer::CullFace::Back);
    vshade::renderer::Renderer::setFrontFace(vshade::renderer::FrontFace::CounterClockwise);
    vshade::renderer::Renderer::setPolygonMode(vshade::renderer::PolygonMode::Fill);
    vshade::renderer::Renderer::setDepthTesting(true);
    vshade::renderer::Renderer::setDepthFunction(vshade::renderer::DepthFunction::Less);
    vshade::renderer::Renderer::setDepthWrite(true);
    vshade::renderer::Renderer::setDithering(false);
    vshade::renderer::Renderer::setClearColor(clearColor);
    vshade::renderer::Renderer::clear(
        vshade::renderer::ClearFlags::Color |
        vshade::renderer::ClearFlags::Depth |
        vshade::renderer::ClearFlags::Stencil
    );
}

[[nodiscard]] vshade::tests::visual::Image captureFramebuffer(
    const vshade::renderer::Framebuffer& framebuffer
) {
    vshade::tests::visual::Image image{
        framebuffer.width(),
        framebuffer.height(),
        framebuffer.readPixels(),
    };
    vshade::renderer::Framebuffer::unbind();
    return image;
}

void checkGoldenImage(
    const vshade::tests::visual::Image& actual,
    const std::string_view imageName
) {
    const std::string filename(imageName);
    const std::filesystem::path goldenPath =
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "render" / (filename + ".png");
    const std::filesystem::path outputDirectory =
        std::filesystem::path(VSHADE_RENDER_OUTPUT_DIR) / filename;

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
    INFO("Test artifacts: " << outputDirectory.string());
    CHECK(result.passed);
}

[[nodiscard]] std::array<std::uint8_t, 8 * 8 * 4> checkerboardPixels() {
    std::array<std::uint8_t, 8 * 8 * 4> pixels{};
    for (std::size_t y = 0; y < 8; ++y) {
        for (std::size_t x = 0; x < 8; ++x) {
            const bool alternate = ((x / 2) + (y / 2)) % 2 != 0;
            const std::array<std::uint8_t, 4> color = alternate
                ? std::array<std::uint8_t, 4>{55, 75, 170, 255}
                : std::array<std::uint8_t, 4>{245, 185, 45, 255};
            const std::size_t offset = (y * 8 + x) * 4;
            for (std::size_t channel = 0; channel < color.size(); ++channel) {
                pixels[offset + channel] = color[channel];
            }
        }
    }
    return pixels;
}

} // namespace

TEST_CASE("Offscreen colored triangle matches its golden image", "[renderer][visual][opengl]") {
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
    beginOffscreenFrame(framebuffer, {0.04F, 0.06F, 0.10F, 1.0F});
    shader.bind();
    vshade::renderer::Renderer::drawIndexed(*vertexArray);

    const vshade::tests::visual::Image actual = captureFramebuffer(framebuffer);

    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().indexCount == 3);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 1);

    vshade::renderer::Renderer::beginFrame();
    vshade::renderer::Renderer::setDepthWrite(false);
    vshade::renderer::Renderer::drawArrays(
        *vertexArray,
        vshade::renderer::PrimitiveTopology::Triangles,
        vertices.size()
    );
    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().vertexCount == 3);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 1);
    vshade::renderer::Renderer::setDepthWrite(true);

    checkGoldenImage(actual, "colored_triangle");
}

TEST_CASE("Offscreen textured quad matches its golden image", "[renderer][visual][opengl]") {
    HiddenRenderContext context;

    const vshade::renderer::Texture2D sandboxTexture =
        vshade::renderer::Texture2D::fromFile(
            std::filesystem::path(VSHADE_SANDBOX_TEXTURE_DIR) / "checkerboard.ppm",
            vshade::renderer::TextureFilter::Nearest,
            vshade::renderer::TextureWrap::ClampToEdge
        );
    CHECK(sandboxTexture.width() == 4);
    CHECK(sandboxTexture.height() == 4);
    CHECK(sandboxTexture.format() == vshade::renderer::TextureFormat::RGBA8);

    constexpr std::array<TexturedVertex, 4> vertices{{
        {{-0.75F, -0.65F, 0.0F}, {0.0F, 0.0F}},
        {{0.75F, -0.65F, 0.0F}, {1.0F, 0.0F}},
        {{0.75F, 0.65F, 0.0F}, {1.0F, 1.0F}},
        {{-0.75F, 0.65F, 0.0F}, {0.0F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 3, 0};

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
    });
    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    vshade::renderer::VertexArray vertexArray;
    vertexArray.addVertexBuffer(std::move(vertexBuffer));
    vertexArray.setIndexBuffer(std::move(indexBuffer));

    vshade::renderer::Shader shader(
        "visual-test-textured-quad",
        R"glsl(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureCoordinate;

out vec2 vertexTextureCoordinate;

void main() {
    vertexTextureCoordinate = textureCoordinate;
    gl_Position = vec4(position, 1.0);
}
)glsl",
        R"glsl(#version 330 core
in vec2 vertexTextureCoordinate;

uniform sampler2D image;
out vec4 fragmentColor;

void main() {
    fragmentColor = texture(image, vertexTextureCoordinate);
}
)glsl"
    );

    const auto pixels = checkerboardPixels();
    const std::filesystem::path textureInputDirectory =
        std::filesystem::path(VSHADE_RENDER_OUTPUT_DIR) / "texture_input";
    const std::filesystem::path texturePath = textureInputDirectory / "checkerboard.png";
    CHECK_THROWS_AS(
        vshade::renderer::Texture2D::fromFile(textureInputDirectory / "missing.png"),
        std::runtime_error
    );
    vshade::tests::visual::writePng(
        texturePath,
        {
            8,
            8,
            std::vector<std::uint8_t>(pixels.begin(), pixels.end()),
        }
    );

    const vshade::renderer::Texture2D texture = vshade::renderer::Texture2D::fromFile(
        texturePath,
        vshade::renderer::TextureFilter::Nearest,
        vshade::renderer::TextureWrap::ClampToEdge
    );
    CHECK(texture.width() == 8);
    CHECK(texture.height() == 8);
    CHECK(texture.format() == vshade::renderer::TextureFormat::RGBA8);

    vshade::renderer::Framebuffer framebuffer(renderWidth, renderHeight);
    beginOffscreenFrame(framebuffer, {0.03F, 0.04F, 0.07F, 1.0F});
    shader.setInt("image", 0);
    texture.bind(0);
    vshade::renderer::Renderer::drawIndexed(vertexArray);

    const vshade::tests::visual::Image actual = captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().indexCount == 6);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 2);
    checkGoldenImage(actual, "textured_quad");
}

TEST_CASE("Offscreen rotating cube snapshot matches its golden image", "[renderer][visual][opengl]") {
    HiddenRenderContext context;

    constexpr std::array<ColoredVertex, 8> vertices{{
        {{-0.70F, -0.70F, -0.70F}, {0.15F, 0.25F, 0.85F}},
        {{0.70F, -0.70F, -0.70F}, {0.85F, 0.25F, 0.20F}},
        {{0.70F, 0.70F, -0.70F}, {0.95F, 0.80F, 0.20F}},
        {{-0.70F, 0.70F, -0.70F}, {0.20F, 0.80F, 0.35F}},
        {{-0.70F, -0.70F, 0.70F}, {0.25F, 0.45F, 1.0F}},
        {{0.70F, -0.70F, 0.70F}, {0.95F, 0.35F, 0.65F}},
        {{0.70F, 0.70F, 0.70F}, {1.0F, 0.90F, 0.40F}},
        {{-0.70F, 0.70F, 0.70F}, {0.25F, 0.95F, 0.75F}},
    }};
    constexpr std::array<std::uint32_t, 36> indices{
        0, 3, 2, 2, 1, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 2, 6, 6, 5, 1,
        0, 1, 5, 5, 4, 0,
        3, 7, 6, 6, 2, 3,
    };

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"color", vshade::renderer::ShaderDataType::Float3},
    });
    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    vshade::renderer::VertexArray vertexArray;
    vertexArray.addVertexBuffer(std::move(vertexBuffer));
    vertexArray.setIndexBuffer(std::move(indexBuffer));

    vshade::renderer::Shader shader(
        "visual-test-rotating-cube",
        R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vertexColor;

void main() {
    vertexColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl",
        R"glsl(#version 330 core
in vec3 vertexColor;

out vec4 fragmentColor;

void main() {
    fragmentColor = vec4(vertexColor, 1.0);
}
)glsl"
    );

    // This fixed angle represents one deterministic frame of the cube's rotation.
    const vshade::math::Mat4 model = vshade::math::composeTransform(
        {0.0F, 0.0F, 0.0F},
        vshade::math::fromEuler({0.42F, 0.68F, 0.12F}),
        {1.0F, 1.0F, 1.0F}
    );
    const vshade::math::Mat4 projection =
        vshade::math::perspective(0.785398163F, 1.0F, 0.1F, 100.0F);
    const vshade::math::Mat4 view = vshade::math::lookAt(
        {3.2F, 2.4F, 4.2F},
        {0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}
    );

    vshade::renderer::Framebuffer framebuffer(renderWidth, renderHeight);
    beginOffscreenFrame(framebuffer, {0.025F, 0.035F, 0.06F, 1.0F});
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    vshade::renderer::Renderer::drawIndexed(vertexArray);

    const vshade::tests::visual::Image actual = captureFramebuffer(framebuffer);
    CHECK(vshade::renderer::Renderer::stats().drawCalls == 1);
    CHECK(vshade::renderer::Renderer::stats().indexCount == 36);
    CHECK(vshade::renderer::Renderer::stats().triangleCount == 12);
    checkGoldenImage(actual, "rotating_cube");
}

TEST_CASE("Image comparison reports errors and writes failure artifacts", "[renderer][visual]") {
    const std::filesystem::path outputDirectory =
        std::filesystem::path(VSHADE_RENDER_OUTPUT_DIR) / "comparison_utility";
    const std::filesystem::path temporaryGolden = outputDirectory / "golden.png";

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

}
