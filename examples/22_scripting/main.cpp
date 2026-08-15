#include <common/ExampleSupport.hpp>

#include "assets/BounceScript.hpp"
#include "assets/SpinScript.hpp"

class ScriptingExample final : public vshade::Application {
public:
    ScriptingExample() : Application(vshade::examples::config("22 - Scripting")) {}

protected:
    void onStart() override {
        // Registration connects each compiled C++ type to the stable name that
        // ScriptComponent stores in scenes and serialized scene files.
        types()
            .script<vshade::examples::scripts::SpinScript>("Example.Spin")
            .script<vshade::examples::scripts::BounceScript>("Example.Bounce");

        auto camera = m_scene.create("Camera");
        camera.transform().setPosition({0.0F, 0.0F, 3.0F});
        auto& settings = camera.add<vshade::scene::CameraComponent>();
        settings.projection = vshade::scene::CameraProjection::Orthographic;
        settings.orthographicHeight = 5.0F;

        auto spinner = m_scene.create("Spin script");
        spinner.transform().setPosition({-2.2F, 0.0F, 0.0F});
        spinner.transform().setScale({1.4F, 0.45F, 1.0F});
        spinner.add<vshade::scene::SpriteRendererComponent>().color =
            {0.15F, 0.72F, 1.0F, 1.0F};
        spinner.add<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{
                {.typeName = "Example.Spin"},
            }}
        );

        auto bouncer = m_scene.create("Bounce script");
        bouncer.transform().setScale({0.8F, 0.8F, 1.0F});
        bouncer.add<vshade::scene::SpriteRendererComponent>().color =
            {1.0F, 0.72F, 0.12F, 1.0F};
        bouncer.add<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{
                {.typeName = "Example.Bounce"},
            }}
        );

        auto combined = m_scene.create("Two scripts");
        combined.transform().setPosition({2.2F, 0.0F, 0.0F});
        combined.transform().setScale({1.4F, 0.45F, 1.0F});
        combined.add<vshade::scene::SpriteRendererComponent>().color =
            {0.92F, 0.22F, 0.58F, 1.0F};
        combined.add<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{
                {.typeName = "Example.Spin"},
                {.typeName = "Example.Bounce"},
            }}
        );

        playScene(m_scene);
    }

    void onUpdate(float) override {
        vshade::examples::closeOnEscape(*this);
    }

private:
    vshade::Scene m_scene{"External native scripts"};
};

VSHADE_GAME(ScriptingExample)
