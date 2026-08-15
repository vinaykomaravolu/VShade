#include <vshade/Game.hpp>

#include <core/Log.hpp>
#include <input/Input.hpp>
#include <renderer/DebugDraw.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace {

constexpr float ballRadius = 0.35F;
constexpr float ballModelScale = ballRadius / 100.0F;

const std::array<vshade::math::Vec3, 6> spawnPositions{
    vshade::math::Vec3{-1.35F, 2.0F, 0.2F},
    vshade::math::Vec3{-0.75F, 3.1F, -0.35F},
    vshade::math::Vec3{-0.15F, 4.2F, 0.25F},
    vshade::math::Vec3{0.45F, 5.3F, -0.25F},
    vshade::math::Vec3{1.05F, 6.4F, 0.35F},
    vshade::math::Vec3{1.45F, 7.5F, -0.15F},
};

const std::array<vshade::math::Vec3, 6> initialVelocities{
    vshade::math::Vec3{-0.75F, 0.0F, 0.15F},
    vshade::math::Vec3{-0.5F, 0.0F, -0.2F},
    vshade::math::Vec3{-0.25F, 0.0F, 0.25F},
    vshade::math::Vec3{0.25F, 0.0F, -0.25F},
    vshade::math::Vec3{0.5F, 0.0F, 0.2F},
    vshade::math::Vec3{0.75F, 0.0F, -0.15F},
};

const std::array<vshade::math::Vec3, 6> lightColors{
    vshade::math::Vec3{1.0F, 0.18F, 0.12F},
    vshade::math::Vec3{1.0F, 0.55F, 0.12F},
    vshade::math::Vec3{0.3F, 1.0F, 0.25F},
    vshade::math::Vec3{0.15F, 0.65F, 1.0F},
    vshade::math::Vec3{0.45F, 0.25F, 1.0F},
    vshade::math::Vec3{1.0F, 0.2F, 0.75F},
};

class SandboxApplication final : public vshade::Application {
public:
    SandboxApplication()
        : Application({.window = {
              .title = "VShade - Falling Balls",
              .width = 1280,
              .height = 720,
              .fullscreen = false,
              .vsync = true,
          }}),
          m_scene("Falling Balls") {}

protected:
    void onStart() override {
        const std::filesystem::path directory = VSHADE_SANDBOX_ASSET_DIR;
        const auto ballModel = assets().reference<vshade::renderer::Model>(
            directory / "ball.glb"
        );
        const auto platformModel = assets().reference<vshade::renderer::Model>(
            directory / "round_platform.glb"
        );

        buildScene(ballModel, platformModel);
        audio().setMasterVolume(0.65F);
        audio().playMusic(directory / "retroloop.mp3", {
            .volume = 0.7F,
            .looping = true,
        });

        runtime().addSystem(
            vshade::scene::SceneRuntimePhase::BeforeRender,
            [this](vshade::Scene&, float) { drawPlatformCollider(); }
        );
        playScene(m_scene);

        GAME_INFO("SceneRuntime now owns physics, lighting, models, audio, and debug flush");
        GAME_INFO("Controls: R resets balls, Escape exits");
    }

    void onUpdate(float) override {
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::Escape)) {
            close();
        }
        if (vshade::input::Input::isKeyPressed(vshade::input::KeyCode::R)) {
            resetBalls();
        }
    }

private:
    void buildScene(
        const vshade::asset::AssetReference<vshade::renderer::Model>& ballModel,
        const vshade::asset::AssetReference<vshade::renderer::Model>& platformModel
    ) {
        m_scene.setEnvironment({
            .ambientColor = {0.28F, 0.34F, 0.52F},
            .ambientIntensity = 0.16F,
        });

        auto camera = m_scene.create("Camera");
        const vshade::math::Vec3 cameraPosition{7.0F, 5.5F, 8.0F};
        camera.transform().setPosition(cameraPosition);
        camera.transform().setRotation(vshade::math::lookRotation(
            vshade::math::Vec3{0.0F, 2.6F, 0.0F} - cameraPosition
        ));
        camera.add<vshade::scene::CameraComponent>();
        camera.add<vshade::scene::AudioListenerComponent>();

        auto sun = m_scene.create("Sun");
        sun.add<vshade::scene::LightComponent>(
            vshade::renderer::DirectionalLight{
                .direction = {-0.45F, -1.0F, -0.35F},
                .color = {1.0F, 0.95F, 0.86F},
                .intensity = 0.8F,
            },
            true
        );

        m_platform = m_scene.create("Round Platform");
        m_platform.add<vshade::scene::ModelRendererComponent>(platformModel, true);
        m_platform.add<vshade::scene::RigidBody3DComponent>().settings.type =
            vshade::physics::BodyType::Static;
        m_platform.add<vshade::scene::Collider3DComponent>(
            vshade::scene::Collider3DComponent{
                .shape = vshade::physics::BoxShape3D{{2.45F, 0.09F, 2.45F}},
                .material = {.friction = 0.8F, .restitution = 0.1F},
            }
        );

        for (std::size_t index = 0; index < spawnPositions.size(); ++index) {
            auto ball = m_scene.create("Ball " + std::to_string(index + 1));
            ball.transform().setPosition(spawnPositions[index]);
            ball.transform().setScale(vshade::math::Vec3{ballModelScale});
            ball.add<vshade::scene::ModelRendererComponent>(ballModel, true);
            auto& body = ball.add<vshade::scene::RigidBody3DComponent>().settings;
            body.type = vshade::physics::BodyType::Dynamic;
            body.linearVelocity = initialVelocities[index];
            body.continuousCollision = true;
            ball.add<vshade::scene::Collider3DComponent>(
                vshade::scene::Collider3DComponent{
                    .shape = vshade::physics::SphereShape3D{ballRadius},
                    .material = {.friction = 0.45F, .restitution = 0.55F},
                }
            );
            ball.add<vshade::scene::LightComponent>(
                vshade::renderer::PointLight{
                    .color = lightColors[index],
                    .intensity = 4.5F,
                    .range = 4.5F,
                },
                true
            );
            m_balls.push_back(ball);
        }
    }

    void resetBalls() {
        for (std::size_t index = 0; index < m_balls.size(); ++index) {
            m_balls[index].transform().setPosition(spawnPositions[index]);
            m_balls[index].transform().setRotation(vshade::math::identity());
        }
        runtime().physics3D().rebuild(m_scene);
    }

    void drawPlatformCollider() const {
        const auto& collider = m_platform.get<vshade::scene::Collider3DComponent>();
        const auto* box = std::get_if<vshade::physics::BoxShape3D>(&collider.shape);
        if (box == nullptr) return;
        const auto center = m_platform.transform().position() + collider.offset;
        vshade::renderer::DebugDraw::box(
            {.minimum = center - box->halfExtents,
             .maximum = center + box->halfExtents},
            {0.15F, 1.0F, 0.25F, 1.0F}
        );
    }

    vshade::Scene m_scene;
    vshade::Entity m_platform;
    std::vector<vshade::Entity> m_balls;
};

} // namespace

VSHADE_GAME(SandboxApplication)
