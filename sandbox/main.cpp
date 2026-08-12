#include <core/application.hpp>
#include <core/assert.hpp>
#include <core/entrypoint.hpp>
#include <core/log.hpp>
#include <core/time.hpp>
#include <input/input.hpp>
#include <math/quaternion.hpp>
#include <math/transform.hpp>
#include <renderer/buffer.hpp>
#include <renderer/camera.hpp>
#include <renderer/cameracontroller.hpp>
#include <renderer/material.hpp>
#include <renderer/mesh.hpp>
#include <renderer/renderer.hpp>
#include <renderer/renderer3d.hpp>
#include <renderer/shader.hpp>
#include <renderer/texture.hpp>
#include <renderer/vertexarray.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>

namespace {

[[nodiscard]] std::unique_ptr<vshade::renderer::Mesh> createCubeMesh() {
    using vshade::renderer::MeshVertex;

    // A cube needs separate vertices per face because every face has a
    // different normal and its own complete set of texture coordinates.
    constexpr std::array<MeshVertex, 24> vertices{{
        // Front (+Z)
        {{-0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
        // Back (-Z)
        {{0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F}},
        // Left (-X)
        {{-0.7F, -0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{-0.7F, -0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{-0.7F, 0.7F, 0.7F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Right (+X)
        {{0.7F, -0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
        {{0.7F, 0.7F, 0.7F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
        // Top (+Y)
        {{-0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, 0.7F, 0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, 0.7F, -0.7F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
        // Bottom (-Y)
        {{-0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 0.0F}},
        {{0.7F, -0.7F, -0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},
        {{0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},
        {{-0.7F, -0.7F, 0.7F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
    }};
    constexpr std::array<std::uint32_t, 36> indices{
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };

    auto vertexBuffer = std::make_shared<vshade::renderer::VertexBuffer>(
        vertices.data(),
        sizeof(vertices)
    );
    vertexBuffer->setLayout({
        {"position", vshade::renderer::ShaderDataType::Float3},
        {"normal", vshade::renderer::ShaderDataType::Float3},
        {"textureCoordinate", vshade::renderer::ShaderDataType::Float2},
    });

    auto indexBuffer = std::make_shared<vshade::renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    auto vertexArray = std::make_shared<vshade::renderer::VertexArray>();
    vertexArray->addVertexBuffer(std::move(vertexBuffer));
    vertexArray->setIndexBuffer(std::move(indexBuffer));

    return std::make_unique<vshade::renderer::Mesh>(std::move(vertexArray));
}

class SandboxApplication final : public vshade::core::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade 2D + 3D Renderer Sandbox",
                  .width = 1280,
                  .height = 720,
                  .fullscreen = false,
                  .vsync = true,
              },
          }) {}

protected:
    void onStart() override {
        // Meshes connect CPU vertex/index data to the engine's vertex-array API.
        m_cubeMesh = createCubeMesh();

        // A material may use Renderer3D's built-in shader, or override it with
        // a shader loaded by the game. The sandbox demonstrates the override.
        const std::filesystem::path shaderDirectory{VSHADE_SANDBOX_SHADER_DIR};
        m_materialShader = std::make_shared<vshade::renderer::Shader>(
            vshade::renderer::Shader::fromFiles(
                "sandbox-material",
                shaderDirectory / "shader.vs",
                shaderDirectory / "shader.fs"
            )
        );

        const std::filesystem::path textureDirectory{VSHADE_SANDBOX_TEXTURE_DIR};
        m_checkerTexture = std::make_shared<vshade::renderer::Texture2D>(
            vshade::renderer::Texture2D::fromFile(
                textureDirectory / "checkerboard.ppm",
                vshade::renderer::TextureFilter::Nearest,
                vshade::renderer::TextureWrap::Repeat
            )
        );

        m_cubeMaterial.setShader(m_materialShader);
        m_cubeMaterial.setAlbedoTexture(m_checkerTexture);
        m_cubeMaterial.setAlbedoColor({0.85F, 0.95F, 1.0F, 1.0F});
        m_cubeMaterial.setRoughness(0.65F);
        m_cubeMaterial.setMetallic(0.05F);
        m_cubeMaterial.setShading(vshade::renderer::MaterialShading::Lit);

        // Start focused on the cube, then let the fly controller update this
        // view from keyboard and mouse input.
        m_camera3D.lookAt(
            {3.2F, 2.2F, 4.2F},
            {0.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F}
        );
        updateCameraProjections(getWindow().width(), getWindow().height());
        m_cameraController =
            std::make_unique<vshade::renderer::CameraController>(m_camera3D);

        vshade::renderer::Renderer3D::setDirectionalLight({
            .direction = {-0.55F, -1.0F, -0.35F},
            .color = {1.0F, 0.94F, 0.82F},
            .intensity = 0.95F,
        });

        GAME_INFO("Sandbox started");
        GAME_INFO(
            "Controls: WASD move, hold right mouse to look, scroll changes speed, Escape exits"
        );
    }

    void onUpdate(const float deltaTime) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }

        ENGINE_ASSERT(
            m_cameraController != nullptr,
            "Camera controller must exist before updating"
        );
        m_cameraController->update(deltaTime);

        // Game state is updated separately from rendering. The render method
        // below only reads this transform and submits it.
        m_cubeAngle += deltaTime * 0.65F;
        m_cubeTransform.setRotation(
            vshade::math::fromEuler({m_cubeAngle * 0.45F, m_cubeAngle, 0.08F})
        );
    }

    void onRender() override {
        vshade::renderer::Renderer::setClearColor({0.025F, 0.035F, 0.06F, 1.0F});
        vshade::renderer::Renderer::clear(
            vshade::renderer::ClearFlags::Color |
            vshade::renderer::ClearFlags::Depth
        );

        ENGINE_ASSERT(m_cubeMesh != nullptr, "Cube mesh must exist before rendering");

        // Render the generated cube using the perspective camera.
        vshade::renderer::Renderer3D::beginScene(m_camera3D);
        vshade::renderer::Renderer3D::drawMesh(
            m_cubeTransform,
            *m_cubeMesh,
            m_cubeMaterial
        );
        vshade::renderer::Renderer3D::endScene();
    }

    void onWindowResize(const std::uint32_t width, const std::uint32_t height) override {
        if (width != 0 && height != 0) {
            updateCameraProjections(width, height);
        }
    }

    void onShutdown() override {
        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::frameCount(),
            vshade::core::Time::elapsedTime()
        );

        // Release GPU-backed objects while Application still owns the active
        // OpenGL context. Material references are cleared first.
        m_cubeMaterial.setShader(nullptr);
        m_cubeMaterial.setAlbedoTexture(nullptr);
        m_cubeMesh.reset();
        m_materialShader.reset();
        m_checkerTexture.reset();
        m_cameraController.reset();
    }

private:
    void updateCameraProjections(const std::uint32_t width, const std::uint32_t height) {
        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        m_camera3D.setPerspective(0.785398163F, aspectRatio, 0.1F, 100.0F);
    }

    std::shared_ptr<vshade::renderer::Shader> m_materialShader;
    std::shared_ptr<vshade::renderer::Texture2D> m_checkerTexture;
    std::unique_ptr<vshade::renderer::Mesh> m_cubeMesh;
    std::unique_ptr<vshade::renderer::CameraController> m_cameraController;
    vshade::renderer::Material m_cubeMaterial;
    vshade::renderer::Camera m_camera3D;
    vshade::math::Transform m_cubeTransform;
    float m_cubeAngle = 0.0F;
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
