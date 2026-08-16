#include <common/ExampleSupport.hpp>

class MultipleLights final : public vshade::Application {
public:
    MultipleLights() : Application(vshade::examples::config("Feature - Multiple Lights")) {}
protected:
    void onStart() override {
        auto camera = m_scene.create("Camera");
        camera.transform().setPosition({0.0F,1.0F,5.0F});
        camera.add<vshade::scene::CameraComponent>();
        auto eye = m_scene.create("Eye");
        eye.transform().setScale({0.01F, 0.01F, 0.01F});
        eye.add<vshade::scene::ModelRendererComponent>(
            assets().reference<vshade::renderer::Model>(vshade::examples::asset("eye.glb")),true);
        constexpr std::array<vshade::math::Vec3,3> colors{
            vshade::math::Vec3{1,0.1F,0.1F}, {0.1F,1,0.1F}, {0.1F,0.2F,1}};
        for (std::size_t i=0;i<colors.size();++i) {
            auto light=m_scene.create("Light");
            light.transform().setPosition({-2.0F+static_cast<float>(i)*2.0F,1.5F,1.0F});
            light.add<vshade::scene::LightComponent>(
                vshade::renderer::PointLight{.color=colors[i],.intensity=4,.range=5},true);
        }
        playScene(m_scene);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
private:
    vshade::Scene m_scene{"Multiple lights"};
};
VSHADE_GAME(MultipleLights)
