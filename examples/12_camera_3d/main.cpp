#include <common/ExampleSupport.hpp>

class Camera3DExample final : public vshade::Application {
public:
    Camera3DExample() : Application(vshade::examples::config("12 - Camera 3D")) {}
protected:
    void onStart() override {
        m_mesh = std::make_unique<vshade::renderer::Mesh>(vshade::examples::cubeMesh());
        m_material.setShading(vshade::renderer::MaterialShading::Lit);
        m_camera = vshade::examples::perspectiveCamera();
        m_controller = std::make_unique<vshade::renderer::CameraController>(m_camera);
    }
    void onUpdate(float dt) override {
        const bool look = vshade::input::Input::isMouseButtonDown(
            vshade::input::MouseButton::Right
        );
        window().setCursorCaptured(look);
        m_controller->update(dt);
        vshade::examples::closeOnEscape(*this);
    }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer3D::setDirectionalLight({});
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        for (int x = -2; x <= 2; ++x) {
            scene.draw(vshade::math::Transform({x * 1.5F, 0.0F, 0.0F}),
                       *m_mesh, m_material);
        }
    }
private:
    std::unique_ptr<vshade::renderer::Mesh> m_mesh;
    vshade::renderer::Material m_material;
    vshade::renderer::Camera m_camera;
    std::unique_ptr<vshade::renderer::CameraController> m_controller;
};
VSHADE_GAME(Camera3DExample)
