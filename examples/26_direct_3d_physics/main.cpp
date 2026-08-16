#include <vshade/Game.hpp>

#include <core/Log.hpp>
#include <input/Input.hpp>
#include <physics/physics3d/PhysicsWorld3D.hpp>
#include <renderer/Camera.hpp>
#include <renderer/Lighting.hpp>
#include <renderer/Material.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Renderer3D.hpp>
#include <renderer/Shader.hpp>
#include <script/NativeScriptSystem.hpp>

#include "assets/HelmetBody.hpp"
#include "assets/HelmetResetScript.hpp"

#include <filesystem>
#include <memory>

class Direct3DPhysicsExample final : public vshade::Application {
public:
    Direct3DPhysicsExample()
        : Application({.window = {
              .title = "26 - Direct 3D Physics",
              .width = 960,
              .height = 540,
              .fullscreen = false,
              .vsync = true,
          }}) {}

protected:
    void onStart() override {
        const std::filesystem::path assetsDirectory =
            std::filesystem::path(VSHADE_EXAMPLE_SOURCE_DIR) / "assets";

        m_helmetModel = assets().loadResource<vshade::renderer::Model>(
            assetsDirectory / "DamagedHelmet.glb"
        ).shared();
        m_platformModel = assets().loadResource<vshade::renderer::Model>(
            assetsDirectory / "round_platform.glb"
        ).shared();
        m_impactClip = assets().loadResource<vshade::audio::AudioClip>(
            assetsDirectory / "sample-3.wav"
        ).shared();
        m_helmetShader = assets().loadResource<vshade::renderer::Shader>(
            assetsDirectory / "shaders/helmet.vs"
        ).shared();
        for (const auto& primitive : m_helmetModel->primitives()) {
            primitive.material->setShader(m_helmetShader);
            primitive.material->parameters().set(
                "accentColor",
                vshade::math::Vec3{0.08F, 0.55F, 1.0F}
            );
        }

        m_camera.setPerspective(0.785398163F, 16.0F / 9.0F, 0.05F, 100.0F);
        m_camera.lookAt(
            {6.0F, 4.5F, 7.0F},
            {0.0F, 1.8F, 0.0F},
            {0.0F, 1.0F, 0.0F}
        );

        m_lighting.setAmbientLight({
            .color = {0.35F, 0.45F, 0.75F},
            .intensity = 0.22F,
        });
        m_lighting.addDirectionalLight({
            .direction = {-0.5F, -1.0F, -0.3F},
            .color = {1.0F, 0.94F, 0.84F},
            .intensity = 1.4F,
        });

        m_platformBody = m_physics.createBody(
            vshade::physics::PhysicsBody3DSettings{
                .type = vshade::physics::BodyType::Static,
                .position = {0.0F, 0.0F, 0.0F},
            },
            vshade::physics::BoxShape3D{{2.45F, 0.09F, 2.45F}},
            vshade::physics::PhysicsMaterial3D{
                .friction = 0.8F,
                .restitution = 0.1F,
            }
        );
        m_helmetBody = std::make_unique<vshade::examples::direct3d::HelmetBody>(
            m_physics,
            vshade::math::Vec3{0.0F, 5.0F, 0.0F}
        );
        m_helmetTransform.setScale({0.65F, 0.65F, 0.65F});
        m_helmetBody->synchronize(m_helmetTransform);

        m_physics.setContactListener(
            [this](const vshade::physics::ContactEvent3D& event) {
                const bool helmetHitPlatform =
                    (event.firstBody == m_helmetBody->body().id() &&
                     event.secondBody == m_platformBody.id()) ||
                    (event.secondBody == m_helmetBody->body().id() &&
                     event.firstBody == m_platformBody.id());
                if (helmetHitPlatform &&
                    event.phase == vshade::physics::ContactPhase::Began &&
                    !m_playedImpact) {
                    m_impactVoice = audio().play(m_impactClip, {.volume = 0.55F});
                    m_playedImpact = true;
                    GAME_INFO("Helmet hit the platform");
                }
            }
        );

        // Without playScene(), the application owns registration, attachment,
        // updates, and shutdown for its native-script system.
        scripts().registerType<vshade::examples::direct3d::HelmetResetScript>(
            "Example.DirectHelmetReset"
        );
        auto scriptHost = m_scriptScene.create("Direct helmet controls");
        scriptHost.add<vshade::examples::direct3d::HelmetResetAction>(
            vshade::examples::direct3d::HelmetResetAction{
                .reset = [this] { resetHelmet(); },
            }
        );
        scriptHost.add<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{
                {.typeName = "Example.DirectHelmetReset"},
            }}
        );
        m_scriptSystem = std::make_unique<vshade::script::NativeScriptSystem>(scripts());
        m_scriptSystem->attachScene(m_scriptScene, services(), runtime());

        GAME_INFO("Direct example: R resets the helmet, Escape exits");
    }

    void onFixedUpdate(const float fixedDeltaTime) override {
        m_scriptSystem->fixedUpdate(fixedDeltaTime);
        m_physics.step(fixedDeltaTime);
        m_helmetBody->synchronize(m_helmetTransform);
    }

    void onUpdate(const float deltaTime) override {
        using vshade::input::Input;
        using vshade::input::KeyCode;
        m_elapsedTime += deltaTime;
        m_scriptSystem->update(deltaTime);
        if (Input::isKeyPressed(KeyCode::Escape)) close();
    }

    void onRender() override {
        vshade::renderer::Renderer::setClearColor({0.025F, 0.035F, 0.075F, 1.0F});
        vshade::renderer::Renderer::clear(
            vshade::renderer::ClearFlags::Color |
            vshade::renderer::ClearFlags::Depth
        );

        vshade::renderer::Renderer3D::setLighting(m_lighting);
        vshade::renderer::Renderer3D::beginScene(m_camera);
        vshade::renderer::Renderer3D::drawModel(
            m_platformTransform,
            *m_platformModel
        );
        vshade::renderer::DrawParameters helmetParameters;
        helmetParameters.set("time", m_elapsedTime);
        vshade::renderer::Renderer3D::drawModel(
            m_helmetTransform,
            *m_helmetModel,
            helmetParameters
        );
        vshade::renderer::Renderer3D::endScene();
    }

    void onShutdown() override {
        m_scriptSystem->detachScene();
        m_scriptSystem.reset();
    }

private:
    void resetHelmet() {
        m_helmetBody->reset();
        m_helmetBody->synchronize(m_helmetTransform);
        m_playedImpact = false;
        GAME_INFO("Helmet reset by HelmetResetScript");
    }

    vshade::physics::PhysicsWorld3D m_physics;
    vshade::physics::PhysicsBody3D m_platformBody;
    std::unique_ptr<vshade::examples::direct3d::HelmetBody> m_helmetBody;

    std::shared_ptr<vshade::renderer::Model> m_helmetModel;
    std::shared_ptr<vshade::renderer::Model> m_platformModel;
    std::shared_ptr<vshade::renderer::Shader> m_helmetShader;
    std::shared_ptr<vshade::audio::AudioClip> m_impactClip;
    vshade::audio::AudioVoice m_impactVoice;

    vshade::renderer::Camera m_camera;
    vshade::renderer::Lighting m_lighting;
    vshade::math::Transform m_platformTransform;
    vshade::math::Transform m_helmetTransform;
    vshade::Scene m_scriptScene{"Manual native-script host"};
    std::unique_ptr<vshade::script::NativeScriptSystem> m_scriptSystem;
    float m_elapsedTime = 0.0F;
    bool m_playedImpact = false;
};

VSHADE_GAME(Direct3DPhysicsExample)
