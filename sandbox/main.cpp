#include <asset/AssetManager.hpp>
#include <core/Application.hpp>
#include <core/Assert.hpp>
#include <core/EntryPoint.hpp>
#include <core/Log.hpp>
#include <core/Time.hpp>
#include <input/Input.hpp>
#include <math/Quaternion.hpp>
#include <math/Transform.hpp>
#include <renderer/Camera.hpp>
#include <renderer/CameraController.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Renderer3D.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>

namespace {

class SandboxApplication final : public vshade::core::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade - A Beautiful Game",
                  .width = 1280,
                  .height = 720,
                  .fullscreen = false,
                  .vsync = true,
              },
          }) {}

protected:
    void onStart() override {
        // AssetManager installs ModelLoader automatically. ModelLoader uses
        // fastgltf to import every mesh primitive, material value, and node
        // transform from the binary glTF file.
        const std::filesystem::path modelPath =
            std::filesystem::path(VSHADE_SANDBOX_ASSET_DIR) / "ABeautifulGame.glb";
        const auto modelHandle =
            m_assets.load<vshade::renderer::Model>(modelPath);
        m_model = m_assets.get(modelHandle);
        ENGINE_ASSERT(m_model != nullptr, "A Beautiful Game model must load");

        // The chessboard is centered at the origin and is about 0.7 units
        // across. Start with an elevated three-quarter view of the full board.
        m_camera.lookAt(
            {0.8F, 0.65F, 0.8F},
            {0.0F, 0.06F, 0.0F},
            {0.0F, 1.0F, 0.0F}
        );
        updateProjection(getWindow().width(), getWindow().height());
        m_cameraController = std::make_unique<vshade::renderer::CameraController>(
            m_camera,
            vshade::renderer::CameraControllerConfig{
                .movementSpeed = 0.75F,
                .mouseSensitivity = 0.002F,
                .scrollSpeedStep = 0.5F,
                .requireRightMouseButton = true,
            }
        );

        vshade::renderer::Renderer3D::setDirectionalLight({
            .direction = {-0.45F, -1.0F, -0.35F},
            .color = {1.0F, 0.95F, 0.86F},
            .intensity = 1.15F,
        });

        GAME_INFO(
            "Loaded ABeautifulGame.glb: {} primitives, {} nodes",
            m_model->primitives().size(),
            m_model->nodes().size()
        );
        GAME_INFO(
            "Controls: WASD move, hold right mouse to look, mouse wheel changes "
            "speed, Space toggles rotation, P toggles wireframe, Escape exits"
        );
    }

    void onUpdate(const float deltaTime) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::P)) {
            m_wireframe = !m_wireframe;
        }
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Space)) {
            m_rotateModel = !m_rotateModel;
        }

        ENGINE_ASSERT(
            m_cameraController != nullptr,
            "Camera controller must exist before updating"
        );
        const bool captureMouse = vshade::input::Input::isMouseButtonDown(
            vshade::input::MouseButton::Right
        );
        getWindow().setCursorCaptured(captureMouse);
        m_cameraController->update(deltaTime);

        if (m_rotateModel) {
            m_modelAngle += deltaTime * 0.35F;
        }
        m_modelTransform.setRotation(
            vshade::math::fromEuler({0.0F, m_modelAngle, 0.0F})
        );
    }

    void onRender() override {
        ENGINE_ASSERT(m_model != nullptr, "Model must exist before rendering");

        vshade::renderer::Renderer::setClearColor({0.025F, 0.035F, 0.06F, 1.0F});
        vshade::renderer::Renderer::clear(
            vshade::renderer::ClearFlags::Color |
            vshade::renderer::ClearFlags::Depth
        );

        // Demonstrate temporary low-level pipeline customization around the
        // high-level model renderer. The guard restores the previous state at
        // the end of this frame.
        auto pipelineGuard = vshade::renderer::Renderer::pushPipelineState();
        auto pipeline = vshade::renderer::Renderer::pipelineState();
        pipeline.polygonMode = m_wireframe
            ? vshade::renderer::PolygonMode::Line
            : vshade::renderer::PolygonMode::Fill;
        pipeline.dithering = false;
        vshade::renderer::Renderer::applyPipelineState(pipeline);

        vshade::renderer::Renderer3D::beginScene(m_camera);
        vshade::renderer::Renderer3D::drawModel(
            m_modelTransform,
            *m_model
        );
        vshade::renderer::Renderer3D::endScene();
    }

    void onWindowResize(
        const std::uint32_t width,
        const std::uint32_t height
    ) override {
        if (width != 0 && height != 0) {
            updateProjection(width, height);
        }
    }

    void onShutdown() override {
        getWindow().setCursorCaptured(false);

        // Release model meshes and other GPU-backed resources while the OpenGL
        // context still exists.
        m_model.reset();
        m_assets.clear();
        m_cameraController.reset();

        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::frameCount(),
            vshade::core::Time::elapsedTime()
        );
    }

private:
    void updateProjection(const std::uint32_t width, const std::uint32_t height) {
        const float aspectRatio = static_cast<float>(width) /
            static_cast<float>(height);
        m_camera.setPerspective(0.785398163F, aspectRatio, 0.01F, 100.0F);
    }

    vshade::asset::AssetManager m_assets;
    std::shared_ptr<vshade::renderer::Model> m_model;
    std::unique_ptr<vshade::renderer::CameraController> m_cameraController;
    vshade::renderer::Camera m_camera;
    vshade::math::Transform m_modelTransform;
    float m_modelAngle = 0.0F;
    bool m_rotateModel = true;
    bool m_wireframe = false;
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
