#include <common/ExampleSupport.hpp>
#include <core/Log.hpp>

class AssetManagerExample final : public vshade::Application {
public:
    AssetManagerExample()
        : Application(vshade::examples::config("21 - Asset Manager")) {}
protected:
    void onStart() override {
        const auto path = vshade::examples::asset("player.png");
        const auto first = assets().loadResource<vshade::renderer::Texture2D>(path);
        const auto second = assets().loadResource<vshade::renderer::Texture2D>(path);
        GAME_INFO("Stable handle: {}; same cached resource: {}",
                  first.handle().id(), first.shared() == second.shared());

        const auto model = assets().reference<vshade::renderer::Model>(
            vshade::examples::asset("DamagedHelmet.glb")
        );
        GAME_INFO("Cataloged model {} without loading it", model.handle().id());
    }
    void onUpdate(float) override { vshade::examples::closeOnEscape(*this); }
    void onRender() override { vshade::examples::clear(); }
};
VSHADE_GAME(AssetManagerExample)
