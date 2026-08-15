#include <common/ExampleSupport.hpp>

class TriangleExample final : public vshade::Application {
public:
    TriangleExample() : Application(vshade::examples::config("02 - Triangle")) {}

protected:
    void onStart() override {
        m_mesh = std::make_unique<vshade::renderer::Mesh>(
            vshade::examples::triangleMesh()
        );
        m_material.setShader(std::make_shared<vshade::renderer::Shader>(
            vshade::renderer::Shader::fromFiles(
                "example-triangle",
                std::filesystem::path(VSHADE_EXAMPLE_SOURCE_DIR) /
                    "assets/shaders/triangle.vs",
                std::filesystem::path(VSHADE_EXAMPLE_SOURCE_DIR) /
                    "assets/shaders/triangle.fs"
            )
        ));
        m_camera.setOrthographic(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);
    }

    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }

    void onRender() override {
        vshade::examples::clear();
        auto scene = vshade::renderer::Renderer3D::scopedScene(m_camera);
        scene.draw(vshade::math::Transform{}, *m_mesh, m_material);
    }

private:
    std::unique_ptr<vshade::renderer::Mesh> m_mesh;
    vshade::renderer::Material m_material;
    vshade::renderer::Camera m_camera;
};

VSHADE_GAME(TriangleExample)
