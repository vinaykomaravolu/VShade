#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class RenderingWorkflows final : public vshade::Application {
public:
    RenderingWorkflows()
        : Application(vshade::examples::config("25 - Rendering Workflows")) {}

protected:
    void onStart() override {
        const std::filesystem::path modelPath =
            vshade::examples::asset("DamagedHelmet.glb");
        m_model = assets().loadResource<vshade::renderer::Model>(modelPath).shared();

        constexpr vshade::math::Vec3 cameraPosition{3.0F, 2.0F, 4.0F};
        m_camera.setPerspective(1.0471975512F, 16.0F / 9.0F, 0.1F, 100.0F);
        m_camera.lookAt(cameraPosition, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});

        m_lighting.setAmbientLight({.color = {0.55F, 0.65F, 1.0F}, .intensity = 0.2F});
        m_lighting.addDirectionalLight({
            .direction = {-0.5F, -1.0F, -0.25F},
            .color = {1.0F, 0.92F, 0.82F},
            .intensity = 2.0F,
        });

        m_scene.setEnvironment({
            .ambientColor = {0.55F, 0.65F, 1.0F},
            .ambientIntensity = 0.2F,
        });
        auto sceneCamera = m_scene.create("Runtime camera");
        sceneCamera.transform().setPosition(cameraPosition);
        sceneCamera.transform().setRotation(vshade::math::lookRotation(-cameraPosition));
        auto& cameraSettings = sceneCamera.add<vshade::scene::CameraComponent>();
        cameraSettings.clearColor = playSceneClearColor;

        m_scene.create("Runtime sun").add<vshade::scene::LightComponent>(
            vshade::renderer::DirectionalLight{
                .direction = {-0.5F, -1.0F, -0.25F},
                .color = {1.0F, 0.92F, 0.82F},
                .intensity = 2.0F,
            },
            true
        );

        m_sceneModel = m_scene.create("Runtime model");
        m_sceneModel.add<vshade::scene::ModelRendererComponent>(
            assets().reference<vshade::renderer::Model>(modelPath),
            true
        );

        select(Workflow::PlayScene);
        GAME_INFO("Press 1: playScene, 2: scopedScene, 3: beginScene/endScene");
    }

    void onUpdate(float deltaTime) override {
        using vshade::input::Input;
        using vshade::input::KeyCode;

        if (Input::isKeyPressed(KeyCode::D1)) select(Workflow::PlayScene);
        if (Input::isKeyPressed(KeyCode::D2)) select(Workflow::ScopedScene);
        if (Input::isKeyPressed(KeyCode::D3)) select(Workflow::ManualScene);

        m_rotation += deltaTime;
        const vshade::math::Quat rotation =
            vshade::math::fromEuler({0.0F, m_rotation, 0.0F});
        m_transform.setRotation(rotation);
        m_sceneModel.transform().setRotation(rotation);
        vshade::examples::closeOnEscape(*this);
    }

    void onRender() override {
        if (m_workflow == Workflow::PlayScene) {
            // SceneRuntime already rendered before Application::onRender().
            return;
        }

        if (m_workflow == Workflow::ScopedScene) {
            vshade::examples::clear(scopedSceneClearColor);
            auto render = vshade::renderer::Renderer3D::scopedScene(m_camera);
            render.setLighting(m_lighting);
            render.draw(m_transform, *m_model);
            return; // The scope still calls endScene().
        }

        vshade::examples::clear(manualSceneClearColor);
        vshade::renderer::Renderer3D::setLighting(m_lighting);
        vshade::renderer::Renderer3D::beginScene(m_camera);
        vshade::renderer::Renderer3D::drawModel(m_transform, *m_model);
        vshade::renderer::Renderer3D::endScene();
    }

private:
    enum class Workflow {
        PlayScene,
        ScopedScene,
        ManualScene,
    };

    void select(const Workflow workflow) {
        if (m_workflow == workflow &&
            (workflow != Workflow::PlayScene || activeScene() == &m_scene)) {
            return;
        }

        m_workflow = workflow;
        if (workflow == Workflow::PlayScene) {
            playScene(m_scene);
            GAME_INFO("Workflow 1: SceneRuntime through playScene(scene)");
        } else {
            stopScene();
            GAME_INFO(
                workflow == Workflow::ScopedScene
                    ? "Workflow 2: Renderer3D::scopedScene(camera)"
                    : "Workflow 3: Renderer3D::beginScene/endScene"
            );
        }
    }

    static constexpr vshade::math::Vec4 playSceneClearColor{
        0.025F, 0.05F, 0.16F, 1.0F
    };
    static constexpr vshade::math::Vec4 scopedSceneClearColor{
        0.025F, 0.13F, 0.075F, 1.0F
    };
    static constexpr vshade::math::Vec4 manualSceneClearColor{
        0.16F, 0.045F, 0.035F, 1.0F
    };

    vshade::Scene m_scene{"Rendering workflows"};
    vshade::Entity m_sceneModel;
    std::shared_ptr<vshade::renderer::Model> m_model;
    vshade::renderer::Camera m_camera;
    vshade::renderer::Lighting m_lighting;
    vshade::math::Transform m_transform;
    Workflow m_workflow = Workflow::ManualScene;
    float m_rotation = 0.0F;
};

VSHADE_GAME(RenderingWorkflows)
