#include <common/ExampleSupport.hpp>

class Transparency final : public vshade::Application {
public:
    Transparency() : Application(vshade::examples::config("Feature - Transparency")) {}
protected:
    void onStart() override { m_camera.setOrthographic(-3,3,-2,2,-1,1); }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer2D::beginScene(m_camera);
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform({-0.6F,0.0F,0.0F}),{1.0F,0.2F,0.1F,0.65F},0);
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform({0.6F,0.0F,0.0F}),{0.1F,0.4F,1.0F,0.65F},1);
        vshade::renderer::Renderer2D::endScene();
    }
private:
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(Transparency)
