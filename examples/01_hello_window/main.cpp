#include <common/ExampleSupport.hpp>

class HelloWindow final : public vshade::Application {
public:
  HelloWindow() : Application(vshade::examples::config("01 - Hello Window")) {}

protected:
  void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
  void onRender() override {
    vshade::examples::clear({0.08F, 0.12F, 0.2F, 1.0F});
  }
};

VSHADE_GAME(HelloWindow)
