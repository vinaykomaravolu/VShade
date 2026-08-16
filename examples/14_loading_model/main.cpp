#include <common/ExampleSupport.hpp>

class LoadingModel final : public vshade::Application {
public:
    LoadingModel() : Application(vshade::examples::config("14 - Loading Model")) {}
protected:
    void onStart() override {
        m_model = assets().loadResource<vshade::renderer::Model>(
            vshade::examples::asset("DamagedHelmet.glb")
        ).shared();
        m_camera = vshade::examples::perspectiveCamera();
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer3D::setDirectionalLight({});
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        scene.draw(vshade::math::Transform{}, *m_model);
    }
private:
    std::shared_ptr<vshade::renderer::Model> m_model;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(LoadingModel)
