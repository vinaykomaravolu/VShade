#include <common/ExampleSupport.hpp>

class RenderToTexture final : public vshade::Application {
public:
    RenderToTexture() : Application(vshade::examples::config("Feature - Render to Texture")) {}
protected:
    void onStart() override {
        m_target=std::make_unique<vshade::renderer::Framebuffer>(512,512);
        m_camera.setOrthographic(-2,2,-2,2,-1,1);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        m_target->bind();
        vshade::examples::clear({0.3F,0.05F,0.1F,1});
        vshade::renderer::Renderer2D::beginScene(m_camera);
        vshade::renderer::Renderer2D::drawQuad(vshade::math::Transform{},
                                               {1,0.8F,0.1F,1});
        vshade::renderer::Renderer2D::endScene();
        vshade::renderer::Framebuffer::unbind();
        vshade::examples::clear();
    }
private:
    std::unique_ptr<vshade::renderer::Framebuffer> m_target;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(RenderToTexture)
