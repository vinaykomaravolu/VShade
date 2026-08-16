#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class ModelHierarchy final : public vshade::Application {
public:
    ModelHierarchy() : Application(vshade::examples::config("16 - Model Hierarchy")) {}
protected:
    void onStart() override {
        m_model = assets().loadResource<vshade::renderer::Model>(
            vshade::examples::asset("visual_model.gltf")
        ).shared();
        m_camera = vshade::examples::perspectiveCamera();
        GAME_INFO("Model has {} primitives and {} nodes", m_model->primitives().size(),
                  m_model->nodes().size());
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        scene.draw(vshade::math::Transform{}, *m_model);
    }
private:
    std::shared_ptr<vshade::renderer::Model> m_model;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(ModelHierarchy)
