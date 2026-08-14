#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <audio/AudioEngine.hpp>
#include <audio/AudioTypes.hpp>
#include <core/Application.hpp>
#include <core/Assert.hpp>
#include <core/EntryPoint.hpp>
#include <core/Log.hpp>
#include <core/Time.hpp>
#include <input/Input.hpp>
#include <physics/PhysicsTypes.hpp>
#include <physics/physics3d/PhysicsBody3D.hpp>
#include <physics/physics3d/PhysicsMaterial3D.hpp>
#include <physics/physics3d/PhysicsShape3D.hpp>
#include <physics/physics3d/PhysicsSystem3D.hpp>
#include <renderer/Camera.hpp>
#include <renderer/CameraController.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Renderer3D.hpp>
#include <scene/Components.hpp>
#include <scene/Entity.hpp>
#include <scene/Scene.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr float ballRadius = 0.35F;
// ball.glb has an imported 100x node scale; this produces a 0.35-unit ball.
constexpr float ballModelScale = ballRadius / 100.0F;

const std::array<vshade::math::Vec3, 6> ballSpawnPositions{
    vshade::math::Vec3{-1.35F, 2.0F, 0.2F},
    vshade::math::Vec3{-0.75F, 3.1F, -0.35F},
    vshade::math::Vec3{-0.15F, 4.2F, 0.25F},
    vshade::math::Vec3{0.45F, 5.3F, -0.25F},
    vshade::math::Vec3{1.05F, 6.4F, 0.35F},
    vshade::math::Vec3{1.45F, 7.5F, -0.15F},
};

const std::array<vshade::math::Vec3, 6> ballInitialVelocities{
    vshade::math::Vec3{-0.75F, 0.0F, 0.15F},
    vshade::math::Vec3{-0.5F, 0.0F, -0.2F},
    vshade::math::Vec3{-0.25F, 0.0F, 0.25F},
    vshade::math::Vec3{0.25F, 0.0F, -0.25F},
    vshade::math::Vec3{0.5F, 0.0F, 0.2F},
    vshade::math::Vec3{0.75F, 0.0F, -0.15F},
};

class SandboxApplication final : public vshade::core::Application {
public:
    SandboxApplication()
        : Application({
              .window = {
                  .title = "VShade - Falling Balls",
                  .width = 1280,
                  .height = 720,
                  .fullscreen = false,
                  .vsync = true,
              },
          }),
          m_scene("Falling Balls") {}

protected:
    void onStart() override {
        const std::filesystem::path assetDirectory = VSHADE_SANDBOX_ASSET_DIR;

        const auto ballModelHandle = m_assets.load<vshade::renderer::Model>(
            assetDirectory / "ball.glb"
        );
        const auto platformModelHandle = m_assets.load<vshade::renderer::Model>(
            assetDirectory / "round_platform.glb"
        );
        const auto musicHandle = m_assets.load<vshade::audio::AudioClip>(
            assetDirectory / "retroloop.mp3"
        );
        m_ballModel = m_assets.get(ballModelHandle);
        m_platformModel = m_assets.get(platformModelHandle);
        m_music = m_assets.get(musicHandle);
        ENGINE_ASSERT(m_ballModel != nullptr, "ball.glb must load");
        ENGINE_ASSERT(m_platformModel != nullptr, "round_platform.glb must load");
        ENGINE_ASSERT(m_music != nullptr, "retroloop.mp3 must load");

        createPhysicsScene();

        m_audio.initialize();
        m_audio.setMasterVolume(0.65F);
        m_audio.play(m_music, {
            .bus = vshade::audio::AudioBus::Music,
            .loadMode = vshade::audio::AudioLoadMode::Stream,
            .volume = 0.7F,
            .looping = true,
        });

        m_camera.lookAt(
            {7.0F, 5.5F, 8.0F},
            {0.0F, 2.6F, 0.0F},
            {0.0F, 1.0F, 0.0F}
        );
        updateProjection(getWindow().width(), getWindow().height());
        m_cameraController = std::make_unique<vshade::renderer::CameraController>(
            m_camera,
            vshade::renderer::CameraControllerConfig{
                .movementSpeed = 3.0F,
                .mouseSensitivity = 0.002F,
                .scrollSpeedStep = 0.5F,
                .requireRightMouseButton = true,
            }
        );

        vshade::renderer::Renderer3D::setDirectionalLight({
            .direction = {-0.45F, -1.0F, -0.35F},
            .color = {1.0F, 0.95F, 0.86F},
            .intensity = 1.2F,
        });

        GAME_INFO(
            "Loaded ball.glb, round_platform.glb, and streaming retroloop.mp3"
        );
        GAME_INFO(
            "Controls: WASD move, right mouse look, R resets balls, "
            "P toggles wireframe, Escape exits"
        );
    }

    void onUpdate(const float deltaTime) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::P)) {
            m_wireframe = !m_wireframe;
        }
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::R)) {
            resetBalls();
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
    }

    void onFixedUpdate(const float fixedDeltaTime) override {
        m_physics.update(m_scene, fixedDeltaTime);
    }

    void onRender() override {
        ENGINE_ASSERT(m_ballModel != nullptr, "Ball model must exist before rendering");
        ENGINE_ASSERT(
            m_platformModel != nullptr,
            "Platform model must exist before rendering"
        );

        vshade::renderer::Renderer::setClearColor({0.025F, 0.035F, 0.06F, 1.0F});
        vshade::renderer::Renderer::clear(
            vshade::renderer::ClearFlags::Color |
            vshade::renderer::ClearFlags::Depth
        );

        auto pipelineGuard = vshade::renderer::Renderer::pushPipelineState();
        auto pipeline = vshade::renderer::Renderer::pipelineState();
        pipeline.polygonMode = m_wireframe
            ? vshade::renderer::PolygonMode::Line
            : vshade::renderer::PolygonMode::Fill;
        pipeline.dithering = false;
        vshade::renderer::Renderer::applyPipelineState(pipeline);

        vshade::renderer::Renderer3D::beginScene(m_camera);
        vshade::renderer::Renderer3D::drawModel(
            m_platform.component<vshade::scene::TransformComponent>().transform,
            *m_platformModel
        );
        for (const vshade::scene::Entity ball : m_balls) {
            vshade::renderer::Renderer3D::drawModel(
                ball.component<vshade::scene::TransformComponent>().transform,
                *m_ballModel
            );
        }
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
        m_physics.clear();
        if (m_music) {
            m_audio.stop(m_music);
        }
        m_audio.shutdown();

        // Release audio and GPU-backed assets while their systems are alive.
        m_music.reset();
        m_ballModel.reset();
        m_platformModel.reset();
        m_assets.clear();
        m_cameraController.reset();

        GAME_INFO(
            "Sandbox stopped after {} frames ({:.2f} seconds)",
            vshade::core::Time::frameCount(),
            vshade::core::Time::elapsedTime()
        );
    }

private:
    void createPhysicsScene() {
        m_platform = m_scene.createEntity("Round Platform");
        auto& platformTransform =
            m_platform.component<vshade::scene::TransformComponent>().transform;
        // round_platform.glb already contains its own 0.01 import scale and
        // renders about five units across without another correction here.
        platformTransform.setScale({1.0F, 1.0F, 1.0F});
        m_platform.addComponent<vshade::scene::RigidBody3DComponent>().settings.type =
            vshade::physics::BodyType::Static;
        m_platform.addComponent<vshade::scene::Collider3DComponent>(
            vshade::scene::Collider3DComponent{
                .shape = vshade::physics::BoxShape3D{{2.45F, 0.09F, 2.45F}},
                .material = {.friction = 0.8F, .restitution = 0.1F},
            }
        );

        m_balls.reserve(ballSpawnPositions.size());
        for (std::size_t index = 0; index < ballSpawnPositions.size(); ++index) {
            vshade::scene::Entity ball = m_scene.createEntity(
                "Ball " + std::to_string(index + 1)
            );
            auto& transform =
                ball.component<vshade::scene::TransformComponent>().transform;
            transform.setPosition(ballSpawnPositions[index]);
            transform.setScale({ballModelScale, ballModelScale, ballModelScale});

            auto& rigidBody =
                ball.addComponent<vshade::scene::RigidBody3DComponent>().settings;
            rigidBody.type = vshade::physics::BodyType::Dynamic;
            rigidBody.mass = 1.0F;
            rigidBody.linearVelocity = ballInitialVelocities[index];
            rigidBody.linearDamping = 0.05F;
            rigidBody.angularDamping = 0.1F;
            rigidBody.continuousCollision = true;
            ball.addComponent<vshade::scene::Collider3DComponent>(
                vshade::scene::Collider3DComponent{
                    .shape = vshade::physics::SphereShape3D{ballRadius},
                    .material = {.friction = 0.45F, .restitution = 0.55F},
                }
            );
            m_balls.push_back(ball);
        }

        m_physics.rebuild(m_scene);
    }

    void resetBalls() {
        for (std::size_t index = 0; index < m_balls.size(); ++index) {
            auto& transform = m_balls[index]
                .component<vshade::scene::TransformComponent>()
                .transform;
            transform.setPosition(ballSpawnPositions[index]);
            transform.setRotation({1.0F, 0.0F, 0.0F, 0.0F});
        }
        m_physics.rebuild(m_scene);
    }

    void updateProjection(const std::uint32_t width, const std::uint32_t height) {
        const float aspectRatio = static_cast<float>(width) /
            static_cast<float>(height);
        m_camera.setPerspective(0.785398163F, aspectRatio, 0.05F, 100.0F);
    }

    vshade::asset::AssetManager m_assets;
    vshade::scene::Scene m_scene;
    vshade::physics::PhysicsSystem3D m_physics;
    vshade::audio::AudioEngine m_audio;
    std::shared_ptr<vshade::renderer::Model> m_ballModel;
    std::shared_ptr<vshade::renderer::Model> m_platformModel;
    std::shared_ptr<vshade::audio::AudioClip> m_music;
    vshade::scene::Entity m_platform;
    std::vector<vshade::scene::Entity> m_balls;
    std::unique_ptr<vshade::renderer::CameraController> m_cameraController;
    vshade::renderer::Camera m_camera;
    bool m_wireframe = false;
};

} // namespace

SHADE_ENGINE_MAIN(SandboxApplication)
