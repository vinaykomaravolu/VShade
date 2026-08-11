#include <core/application.hpp>
#include <core/assert.hpp>
#include <core/entrypoint.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <input/input.hpp>
#include <math/vector.hpp>
#include <renderer/buffer.hpp>
#include <renderer/renderer.hpp>
#include <renderer/shader.hpp>
#include <renderer/texture.hpp>
#include <renderer/vertexarray.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace {

struct TriangleVertex {
    vshade::math::Vec3 position;
    vshade::math::Vec2 textureCoordinate;
};

class SandboxApplication final : public vshade::core::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade Sandbox",
                  .width = 1920,
                  .height = 1080,
                  .fullscreen = false,
                  .vsync = true,
              },
          }) {}

protected:
    void onStart() override {
        constexpr std::array<TriangleVertex, 3> vertices{{
            {{-0.65F, -0.55F, 0.0F}, {0.0F, 0.0F}},
            {{0.65F, -0.55F, 0.0F}, {1.0F, 0.0F}},
            {{0.0F, 0.65F, 0.0F}, {0.5F, 1.0F}},
        }};
        constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};

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

        m_triangle = std::make_unique<vshade::renderer::VertexArray>();
        m_triangle->addVertexBuffer(std::move(vertexBuffer));
        m_triangle->setIndexBuffer(std::move(indexBuffer));

        const std::filesystem::path shaderDirectory{VSHADE_SANDBOX_SHADER_DIR};
        m_shader = std::make_unique<vshade::renderer::Shader>(
            vshade::renderer::Shader::fromFiles(
                "sandbox-triangle",
                shaderDirectory / "shader.vs",
                shaderDirectory / "shader.fs"
            )
        );

        const std::filesystem::path textureDirectory{VSHADE_SANDBOX_TEXTURE_DIR};
        m_texture = std::make_unique<vshade::renderer::Texture2D>(
            vshade::renderer::Texture2D::fromFile(
                textureDirectory / "checkerboard.ppm",
                vshade::renderer::TextureFilter::Nearest,
                vshade::renderer::TextureWrap::ClampToEdge
            )
        );
        m_shader->setInt("image", 0);

        GAME_INFO("Sandbox started");
    }

    void onUpdate(const float delta_time) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }

        GAME_INFO(
            "Mouse position: {} {}",
            vshade::input::Input::mousePosition().x,
            vshade::input::Input::mousePosition().y
        );

        static_cast<void>(delta_time);
    }

    void onRender() override {
        vshade::renderer::Renderer::setClearColor({0.05F, 0.06F, 0.09F, 1.0F});
        vshade::renderer::Renderer::clear();

        ENGINE_ASSERT(m_shader != nullptr, "Sandbox shader must exist before rendering");
        ENGINE_ASSERT(m_texture != nullptr, "Sandbox texture must exist before rendering");
        ENGINE_ASSERT(m_triangle != nullptr, "Sandbox triangle must exist before rendering");
        m_shader->bind();
        m_texture->bind(0);
        vshade::renderer::Renderer::drawIndexed(*m_triangle);
    }

    void onShutdown() override {
        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::frameCount(),
            vshade::core::Time::elapsedTime()
        );
    }

private:
    std::unique_ptr<vshade::renderer::Shader> m_shader;
    std::unique_ptr<vshade::renderer::Texture2D> m_texture;
    std::unique_ptr<vshade::renderer::VertexArray> m_triangle;
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
