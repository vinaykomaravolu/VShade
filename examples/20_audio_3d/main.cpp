#include <common/ExampleSupport.hpp>

class SpatialAudio final : public vshade::Application {
public:
    SpatialAudio() : Application([] {
        auto value = vshade::examples::config("20 - Audio 3D");
        return value;
    }()) {}
protected:
    void onStart() override {
        auto listener = m_scene.create("Listener");
        listener.transform().setPosition({0.0F,0.0F,5.0F});
        listener.add<vshade::scene::AudioListenerComponent>();

        m_source = m_scene.create("Moving sound");
        auto& source = m_source.add<vshade::scene::AudioSourceComponent>();
        source.clipAsset = assets().reference<vshade::audio::AudioClip>(
            vshade::examples::asset("sample-2.flac")
        );
        source.playOnStart = true;
        source.looping = true;
        source.spatial = true;
        source.attenuation = vshade::audio::AttenuationModel::Inverse;
        source.minimumDistance = 1.0F;
        source.maximumDistance = 12.0F;
        playScene(m_scene);
    }
    void onUpdate(float dt) override {
        m_time += dt;
        m_source.transform().setPosition({std::sin(m_time) * 4.0F,0.0F,0.0F});
        vshade::examples::closeOnEscape(*this);
    }
    void onRender() override { vshade::examples::clear(); }
private:
    vshade::Scene m_scene{"Spatial audio"};
    vshade::Entity m_source;
    float m_time = 0.0F;
};
VSHADE_GAME(SpatialAudio)
