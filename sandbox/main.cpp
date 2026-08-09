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
#include <renderer/vertexarray.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace {

struct TriangleVertex {
    vshade::math::Vec3 position;
    vshade::math::Vec3 color;
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
            {{-0.65F, -0.55F, 0.0F}, {0.95F, 0.25F, 0.20F}},
            {{0.65F, -0.55F, 0.0F}, {0.20F, 0.80F, 0.35F}},
            {{0.0F, 0.65F, 0.0F}, {0.20F, 0.45F, 1.0F}},
        }};
        constexpr std::array<std::uint32_t, 3> indices{0, 1, 2};

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
        ENGINE_ASSERT(m_triangle != nullptr, "Sandbox triangle must exist before rendering");
        m_shader->bind();
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
    std::unique_ptr<vshade::renderer::VertexArray> m_triangle;
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
