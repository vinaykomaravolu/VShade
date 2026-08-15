#include <common/ExampleSupport.hpp>

class CubeExample final : public vshade::Application {
public:
    CubeExample() : Application(vshade::examples::config("11 - Cube")) {}
protected:
    void onStart() override {
        m_mesh = std::make_unique<vshade::renderer::Mesh>(vshade::examples::cubeMesh());
        m_material.setShading(vshade::renderer::MaterialShading::Lit);
        m_material.setAlbedoColor({0.15F, 0.65F, 1.0F, 1.0F});
        m_camera = vshade::examples::perspectiveCamera();
    }
    void onUpdate(float dt) override {
        m_rotation += dt;
        m_transform.setRotation(vshade::math::fromEuler({m_rotation * 0.4F, m_rotation, 0.0F}));
        vshade::examples::closeOnEscape(*this);
    }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer3D::setDirectionalLight({});
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        scene.draw(m_transform, *m_mesh, m_material);
    }
private:
    std::unique_ptr<vshade::renderer::Mesh> m_mesh;
    vshade::renderer::Material m_material;
    vshade::renderer::Camera m_camera;
    vshade::math::Transform m_transform;
    float m_rotation = 0.0F;
};
VSHADE_GAME(CubeExample)
