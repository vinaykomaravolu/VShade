#include <common/ExampleSupport.hpp>

class Renderer2DExample final : public vshade::Application {
public:
    Renderer2DExample() : Application(vshade::examples::config("06 - Renderer 2D")) {}
protected:
    void onStart() override {
        m_texture = assets().loadResource<vshade::renderer::Texture2D>(
            vshade::examples::asset("player.png")
        ).shared();
        m_camera.setOrthographic(-4.0F, 4.0F, -2.25F, 2.25F, -1.0F, 1.0F);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer2D::beginScene(m_camera);
        for (int i = 0; i < 7; ++i) {
            vshade::math::Transform transform({-3.0F + i, 0.0F, 0.0F});
            transform.setScale({0.75F, 0.75F, 1.0F});
            if ((i % 2) == 0) {
                vshade::renderer::Renderer2D::drawQuad(
                    transform,
                    *m_texture,
                    vshade::math::Vec4{1.0F},
                    vshade::math::Vec2{1.0F},
                    i
                );
            } else {
                vshade::renderer::Renderer2D::drawQuad(
                    transform, {0.2F, 0.4F + i * 0.07F, 0.9F, 0.8F}, i
                );
            }
        }
        vshade::renderer::Renderer2D::endScene();
    }
private:
    std::shared_ptr<vshade::renderer::Texture2D> m_texture;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(Renderer2DExample)
