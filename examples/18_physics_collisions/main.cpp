#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class PhysicsCollisions final : public vshade::Application {
public:
    PhysicsCollisions()
        : Application(vshade::examples::config("18 - Physics Collisions")) {}
protected:
    void onStart() override {
        auto trigger = m_scene.create("Trigger");
        trigger.add<vshade::scene::RigidBody2DComponent>().settings.collision =
            {.layer = 0x2U, .mask = 0x1U};
        auto& triggerCollider = trigger.add<vshade::scene::Collider2DComponent>();
        triggerCollider.shape = vshade::physics::BoxShape2D{{2.0F,0.25F}};
        triggerCollider.sensor = true;

        m_ball = m_scene.create("Visitor");
        m_ball.transform().setPosition({0.0F, 3.0F, 0.0F});
        auto& body = m_ball.add<vshade::scene::RigidBody2DComponent>().settings;
        body.type = vshade::physics::BodyType::Dynamic;
        body.collision = {.layer = 0x1U, .mask = 0x2U};
        m_ball.add<vshade::scene::Collider2DComponent>().shape =
            vshade::physics::CircleShape2D{0.4F};

        playScene(m_scene);
        runtime().physics2D().setContactListener(
            [](const vshade::physics::SceneContactEvent2D& event) {
                GAME_INFO("{} contact: {} <-> {}",
                    static_cast<int>(event.phase), event.first.name(), event.second.name());
            }
        );
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override { vshade::examples::clear(); }
private:
    vshade::Scene m_scene{"Collision events"};
    vshade::Entity m_ball;
};
VSHADE_GAME(PhysicsCollisions)
