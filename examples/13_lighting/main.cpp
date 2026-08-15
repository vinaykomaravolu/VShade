#include <common/ExampleSupport.hpp>

class LightingExample final : public vshade::Application {
public:
    LightingExample() : Application(vshade::examples::config("13 - Lighting")) {}
protected:
    void onStart() override {
        m_scene.setEnvironment({.ambientColor = {0.25F,0.35F,0.6F},
                                .ambientIntensity = 0.15F});
        auto camera = m_scene.create("Camera");
        const vshade::math::Vec3 position{4.0F, 3.0F, 6.0F};
        camera.transform().setPosition(position);
        camera.transform().setRotation(vshade::math::lookRotation(-position));
        camera.add<vshade::scene::CameraComponent>();
        m_scene.create("Sun").add<vshade::scene::LightComponent>(
            vshade::renderer::DirectionalLight{.direction={-0.5F,-1.0F,-0.25F}}, true);
        for (int x = -2; x <= 2; ++x) {
            auto light = m_scene.create("Point light");
            light.transform().setPosition({x * 1.4F, 1.0F, 0.0F});
            light.add<vshade::scene::LightComponent>(
                vshade::renderer::PointLight{.color={0.3F + 0.15F*x,0.5F,1.0F},
                                              .intensity=3.0F,.range=4.0F}, true);
        }
        auto model = m_scene.create("Lit model");
        model.transform().setScale({0.01F,0.01F,0.01F});
        model.add<vshade::scene::ModelRendererComponent>(
            assets().reference<vshade::renderer::Model>(vshade::examples::asset("eye.glb")), true);
        playScene(m_scene);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
private:
    vshade::Scene m_scene{"Lighting"};
};
VSHADE_GAME(LightingExample)
