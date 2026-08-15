#include <common/ExampleSupport.hpp>

class MultipleTextures final : public vshade::Application {
public:
    MultipleTextures() : Application(vshade::examples::config("Feature - Multiple Textures")) {}
protected:
    void onStart() override {
        m_player = assets().loadResource<vshade::renderer::Texture2D>(
            vshade::examples::asset("player.png")).shared();
        constexpr std::array<std::uint8_t,16> pixels{
            255,80,60,255, 40,210,130,255, 40,210,130,255, 255,80,60,255};
        m_checker = std::make_shared<vshade::renderer::Texture2D>(
            2,2,vshade::renderer::TextureFormat::RGBA8,pixels.data(),
            vshade::renderer::TextureFilter::Nearest);
        m_camera.setOrthographic(-3.0F,3.0F,-2.0F,2.0F,-1.0F,1.0F);
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer2D::beginScene(m_camera);
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform({-1.2F,0.0F,0.0F}),*m_player);
        vshade::renderer::Renderer2D::drawQuad(
            vshade::math::Transform({1.2F,0.0F,0.0F}),*m_checker);
        vshade::renderer::Renderer2D::endScene();
    }
private:
    std::shared_ptr<vshade::renderer::Texture2D> m_player, m_checker;
    vshade::renderer::Camera m_camera;
};
VSHADE_GAME(MultipleTextures)
