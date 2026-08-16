#include <common/ExampleSupport.hpp>

class AudioExample final : public vshade::Application {
public:
    AudioExample() : Application([] {
        auto result = vshade::examples::config("19 - Audio");
        return result;
    }()) {}
protected:
    void onStart() override {
        audio().setMasterVolume(0.7F);
        audio().setBusVolume(vshade::audio::AudioBus::Music, 0.6F);
        m_voice = audio().play(vshade::examples::asset("sample-1.flac"), {
            .bus = vshade::audio::AudioBus::Music,
            .loadMode = vshade::audio::AudioLoadMode::Stream,
            .looping = true,
        });
    }
    void onUpdate(float) override {
        using namespace vshade::input;
        if (Input::isKeyPressed(KeyCode::Space)) {
            if (m_voice.state() == vshade::audio::AudioState::Paused) m_voice.resume();
            else m_voice.pause();
        }
        if (Input::isKeyPressed(KeyCode::S)) m_voice.stop();
        if (Input::isKeyPressed(KeyCode::P)) {
            audio().playOneShot(vshade::examples::asset("sample-3.wav"));
        }
        vshade::examples::closeOnEscape(*this);
    }
    void onRender() override { vshade::examples::clear(); }
private:
    vshade::audio::AudioVoice m_voice;
};
VSHADE_GAME(AudioExample)
