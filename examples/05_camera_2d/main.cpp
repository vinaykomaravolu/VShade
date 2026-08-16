#include <common/ExampleSupport.hpp>

class Camera2DExample final : public vshade::Application {
public:
    Camera2DExample() : Application(vshade::examples::config("05 - Camera 2D")) {}
protected:
    void onStart() override { updateCamera(); }
    void onUpdate(float dt) override {
        using namespace vshade::input;
        if (Input::isKeyDown(KeyCode::A)) m_position.x -= 2.0F * dt;
        if (Input::isKeyDown(KeyCode::D)) m_position.x += 2.0F * dt;
        if (Input::isKeyDown(KeyCode::W)) m_position.y += 2.0F * dt;
        if (Input::isKeyDown(KeyCode::S)) m_position.y -= 2.0F * dt;
        m_zoom = std::clamp(m_zoom - Input::scrollDelta().y * 0.2F, 0.5F, 4.0F);
        updateCamera();
        vshade::examples::closeOnEscape(*this);
    }
    void onRender() override {
        vshade::examples::clear();
        vshade::renderer::Renderer2D::beginScene(m_camera);
        for (int y = -5; y <= 5; ++y) {
            for (int x = -8; x <= 8; ++x) {
                vshade::math::Transform tile({
                    static_cast<float>(x),
                    static_cast<float>(y),
                    0.0F,
                });
                tile.setScale({0.72F, 0.72F, 1.0F});
                const bool alternating = ((x + y) & 1) == 0;
                const vshade::math::Vec4 color = alternating
                    ? vshade::math::Vec4{0.12F, 0.45F, 0.78F, 1.0F}
                    : vshade::math::Vec4{0.10F, 0.72F, 0.55F, 1.0F};
                vshade::renderer::Renderer2D::drawQuad(tile, color);
            }
        }

        vshade::math::Transform origin;
        origin.setScale({1.2F, 1.2F, 1.0F});
        vshade::renderer::Renderer2D::drawQuad(
            origin,
            {1.0F, 0.25F, 0.12F, 1.0F},
            1
        );
        vshade::renderer::Renderer2D::endScene();
    }
private:
    void updateCamera() {
        m_camera.setOrthographic(-4.0F * m_zoom, 4.0F * m_zoom,
                                 -2.25F * m_zoom, 2.25F * m_zoom, 0.1F, 10.0F);
        m_camera.lookAt({m_position.x, m_position.y, 2.0F},
                        {m_position.x, m_position.y, 0.0F}, {0.0F, 1.0F, 0.0F});
    }
    vshade::renderer::Camera m_camera;
    vshade::math::Vec2 m_position{0.0F};
    float m_zoom = 1.0F;
};
VSHADE_GAME(Camera2DExample)
