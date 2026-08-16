#include <common/ExampleSupport.hpp>

class Simple3DGame final : public vshade::Application {
public:
    Simple3DGame() : Application(vshade::examples::config("24 - Simple 3D Game")) {}
protected:
    void onStart() override {
        const auto ballModel = assets().reference<vshade::renderer::Model>(
            vshade::examples::asset("ball.glb"));
        const auto platformModel = assets().reference<vshade::renderer::Model>(
            vshade::examples::asset("round_platform.glb"));
        m_scene.setEnvironment({.ambientColor={0.3F,0.4F,0.7F},.ambientIntensity=0.18F});

        auto camera = m_scene.create("Camera");
        const vshade::math::Vec3 eye{7.0F,5.5F,8.0F};
        camera.transform().setPosition(eye);
        camera.transform().setRotation(vshade::math::lookRotation(
            vshade::math::Vec3{0.0F,2.5F,0.0F}-eye));
        camera.add<vshade::scene::CameraComponent>();
        camera.add<vshade::scene::AudioListenerComponent>();
        m_scene.create("Sun").add<vshade::scene::LightComponent>(
            vshade::renderer::DirectionalLight{.direction={-0.5F,-1.0F,-0.3F}},true);

        auto platform = m_scene.create("Platform");
        platform.add<vshade::scene::ModelRendererComponent>(platformModel,true);
        platform.add<vshade::scene::RigidBody3DComponent>();
        platform.add<vshade::scene::Collider3DComponent>().shape =
            vshade::physics::BoxShape3D{{2.45F,0.09F,2.45F}};

        for (int i = 0; i < 5; ++i) {
            auto ball = m_scene.create("Ball " + std::to_string(i+1));
            ball.transform().setPosition({-1.2F+i*0.6F,2.0F+i,0.0F});
            ball.transform().setScale({0.0035F,0.0035F,0.0035F});
            ball.add<vshade::scene::ModelRendererComponent>(ballModel,true);
            ball.add<vshade::scene::RigidBody3DComponent>().settings.type =
                vshade::physics::BodyType::Dynamic;
            ball.add<vshade::scene::Collider3DComponent>().shape =
                vshade::physics::SphereShape3D{0.35F};
            ball.add<vshade::scene::LightComponent>(
                vshade::renderer::PointLight{.color={0.2F+i*0.15F,0.5F,1.0F},
                                              .intensity=3.5F,.range=4.0F},true);
        }
        audio().playMusic(vshade::examples::asset("retroloop.mp3"), {.looping=true});
        playScene(m_scene);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
private:
    vshade::Scene m_scene{"Tiny 3D game"};
};
VSHADE_GAME(Simple3DGame)
