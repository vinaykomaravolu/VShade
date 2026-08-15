#include <common/ExampleSupport.hpp>

class TexturedQuad final : public vshade::Application {
public:
    TexturedQuad() : Application(vshade::examples::config("04 - Textured Quad")) {}

protected:
    void onStart() override {
        m_texture = assets().loadResource<vshade::renderer::Texture2D>(
            vshade::examples::asset("player.png")
        ).shared();
        m_camera.setOrthographic(-2.0F, 2.0F, -1.125F, 1.125F, -1.0F, 1.0F);
        m_quad.setScale({1.5F, 1.5F, 1.0F});
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer2D::beginScene(m_camera);
        vshade::renderer::Renderer2D::drawQuad(m_quad, *m_texture);
        vshade::renderer::Renderer2D::endScene();
    }
private:
    std::shared_ptr<vshade::renderer::Texture2D> m_texture;
    vshade::renderer::Camera m_camera;
    vshade::math::Transform m_quad;
};
VSHADE_GAME(TexturedQuad)
