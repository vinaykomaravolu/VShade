#include <common/ExampleSupport.hpp>

class NormalMapping final : public vshade::Application {
public:
    NormalMapping() : Application(vshade::examples::config("Feature - Normal Mapping")) {}
protected:
    void onStart() override {
        m_model=assets().loadResource<vshade::renderer::Model>(
            vshade::examples::asset("eye.glb")).shared();
        m_camera=vshade::examples::perspectiveCamera();
        m_transform.setScale({0.01F, 0.01F, 0.01F});
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer3D::setDirectionalLight({});
        auto scene=vshade::renderer::Renderer3D::scopedScene(m_camera);
        scene.draw(m_transform,*m_model);
    }
private:
    std::shared_ptr<vshade::renderer::Model> m_model;
    vshade::renderer::Camera m_camera;
    vshade::math::Transform m_transform;
};
VSHADE_GAME(NormalMapping)
