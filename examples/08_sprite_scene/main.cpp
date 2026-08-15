#include <common/ExampleSupport.hpp>

class SpriteScene final : public vshade::Application {
public:
    SpriteScene() : Application(vshade::examples::config("08 - Sprite Scene")) {}
protected:
    void onStart() override {
        auto camera = m_scene.create("Camera");
        camera.transform().setPosition({0.0F, 0.0F, 50.0F});
        auto& settings = camera.add<vshade::scene::CameraComponent>();
        settings.projection = vshade::scene::CameraProjection::Orthographic;
        settings.orthographicHeight = 20.0F;

        auto sprite = m_scene.create("Player sprite");
        sprite.transform().setScale({10.0F, 2.0F, 1.0F});
        auto& renderer = sprite.add<vshade::scene::SpriteRendererComponent>();
        renderer.texture = assets().reference<vshade::renderer::Texture2D>(
            vshade::examples::asset("player.png")
        );
        playScene(m_scene);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
private:
    vshade::Scene m_scene{"Sprite scene"};
};
VSHADE_GAME(SpriteScene)
