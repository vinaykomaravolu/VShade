#include <common/ExampleSupport.hpp>

class Physics2DExample final : public vshade::Application {
public:
    Physics2DExample() : Application(vshade::examples::config("17 - Physics 2D")) {}
protected:
    void onStart() override {
        auto camera = m_scene.create("Camera");
        camera.transform().setPosition({0.0F, 2.0F, 5.0F});
        auto& cameraSettings = camera.add<vshade::scene::CameraComponent>();
        cameraSettings.projection = vshade::scene::CameraProjection::Orthographic;
        cameraSettings.orthographicHeight = 8.0F;

        auto floor = m_scene.create("Floor");
        floor.transform().setScale({8.0F, 0.5F, 1.0F});
        floor.add<vshade::scene::SpriteRendererComponent>().color = {0.2F,0.7F,0.3F,1.0F};
        floor.add<vshade::scene::RigidBody2DComponent>();
        floor.add<vshade::scene::Collider2DComponent>().shape =
            vshade::physics::BoxShape2D{{4.0F,0.25F}};

        m_box = m_scene.create("Falling box");
        m_box.transform().setPosition({0.0F, 3.5F, 0.0F});
        m_box.add<vshade::scene::SpriteRendererComponent>().color = {0.2F,0.55F,1.0F,1.0F};
        m_box.add<vshade::scene::RigidBody2DComponent>().settings.type =
            vshade::physics::BodyType::Dynamic;
        m_box.add<vshade::scene::Collider2DComponent>().shape =
            vshade::physics::BoxShape2D{{0.5F,0.5F}};
        playScene(m_scene);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
private:
    vshade::Scene m_scene{"2D physics"};
    vshade::Entity m_box;
};
VSHADE_GAME(Physics2DExample)
