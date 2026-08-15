#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class InputExample final : public vshade::Application {
public:
    InputExample() : Application(vshade::examples::config("03 - Input")) {}

protected:
    void onUpdate(float) override {
        using namespace vshade::input;
        if (Input::isKeyPressed(KeyCode::Space)) {
            m_blue = !m_blue;
            GAME_INFO("Space pressed; mouse=({}, {})", Input::mousePosition().x,
                      Input::mousePosition().y);
        }
        if (Input::isMouseButtonPressed(MouseButton::Left)) {
            GAME_INFO("Left mouse pressed; frame delta=({}, {})",
                      Input::mouseDelta().x, Input::mouseDelta().y);
        }
        if (Input::scrollDelta().y != 0.0F) {
            GAME_INFO("Scrolled {}", Input::scrollDelta().y);
        }
        vshade::examples::closeOnEscape(*this);
    }

    void onRender() override {
        vshade::examples::clear(m_blue
            ? vshade::math::Vec4{0.04F, 0.12F, 0.3F, 1.0F}
            : vshade::math::Vec4{0.25F, 0.07F, 0.06F, 1.0F});
    }

private:
    bool m_blue = true;
};

VSHADE_GAME(InputExample)
