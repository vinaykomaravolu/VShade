#include <common/ExampleSupport.hpp>

class MaterialsExample final : public vshade::Application {
public:
    MaterialsExample() : Application(vshade::examples::config("15 - Materials")) {}
protected:
    void onStart() override {
        m_mesh = std::make_unique<vshade::renderer::Mesh>(vshade::examples::cubeMesh());
        m_camera = vshade::examples::perspectiveCamera();
        for (int index = 0; index < 3; ++index) {
            vshade::renderer::Material material(vshade::renderer::MaterialShading::Lit);
            material.setAlbedoColor({0.2F + index*0.3F, 0.7F-index*0.2F, 0.9F, 1.0F});
            material.setMetallic(index * 0.5F);
            material.setRoughness(1.0F - index * 0.4F);
            m_materials.push_back(std::move(material));
        }
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer3D::setDirectionalLight({});
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        for (std::size_t i = 0; i < m_materials.size(); ++i) {
            scene.draw(vshade::math::Transform({static_cast<float>(i)-1.0F,0.0F,0.0F}),
                       *m_mesh, m_materials[i]);
        }
    }
private:
    std::unique_ptr<vshade::renderer::Mesh> m_mesh;
    std::vector<vshade::renderer::Material> m_materials;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(MaterialsExample)
