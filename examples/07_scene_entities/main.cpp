#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

struct Health { int points = 100; };

class SceneEntities final : public vshade::Application {
public:
    SceneEntities() : Application(vshade::examples::config("07 - Scene Entities")) {}
protected:
    void onStart() override {
        auto player = m_scene.create("Player");
        player.transform().setPosition({1.0F, 2.0F, 0.0F});
        player.add<Health>(75);
        for (const auto [handle, tag, transform] :
             m_scene.view<const vshade::scene::TagComponent,
                          const vshade::scene::TransformComponent>().each()) {
            (void)handle;
            GAME_INFO("{} is at ({}, {})", tag.tag,
                      transform.transform.position().x,
                      transform.transform.position().y);
        }
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override { vshade::examples::clear(); }
private:
    vshade::Scene m_scene{"Entity example"};
};
VSHADE_GAME(SceneEntities)
