#include <asset/AssetManager.hpp>
#include <asset/loader/AudioLoader.hpp>
#include <audio/AudioClip.hpp>
#include <audio/AudioEngine.hpp>
#include <audio/AudioTypes.hpp>
#include <audio/AudioService.hpp>
#include <core/EngineServices.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneAudioSystem.hpp>
#include <scene/SceneRuntime.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <array>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>

static_assert(!std::is_copy_constructible_v<vshade::audio::AudioEngine>);
static_assert(!std::is_copy_assignable_v<vshade::audio::AudioEngine>);
static_assert(std::is_move_constructible_v<vshade::audio::AudioEngine>);
static_assert(std::is_move_assignable_v<vshade::audio::AudioEngine>);

namespace {

const std::filesystem::path testAssetDirectory = VSHADE_TEST_ASSET_DIR;
const std::filesystem::path decodedSample = testAssetDirectory / "sample-3.wav";
const std::filesystem::path streamedSample = testAssetDirectory / "sample-1.flac";

} // namespace

TEST_CASE("Audio components expose safe playback defaults", "[audio][scene]") {
    const vshade::scene::AudioSourceComponent source;
    CHECK_FALSE(source.clip.valid());
    CHECK(source.bus == vshade::audio::AudioBus::SFX);
    CHECK(source.loadMode == vshade::audio::AudioLoadMode::Decode);
    CHECK(source.volume == 1.0F);
    CHECK(source.pitch == 1.0F);
    CHECK_FALSE(source.looping);
    CHECK_FALSE(source.playOnStart);
    CHECK_FALSE(source.spatial);

    const vshade::scene::AudioListenerComponent listener;
    CHECK(listener.active);
}

TEST_CASE("Audio types distinguish buses attenuation and playback state", "[audio]") {
    CHECK(vshade::audio::AudioBus::Master != vshade::audio::AudioBus::Music);
    CHECK(vshade::audio::AttenuationModel::None !=
          vshade::audio::AttenuationModel::Inverse);
    CHECK(vshade::audio::AudioState::Stopped != vshade::audio::AudioState::Playing);
    CHECK(vshade::audio::AudioLoadMode::Decode != vshade::audio::AudioLoadMode::Stream);
}

TEST_CASE("Audio clips load through the default asset manager", "[audio][asset]") {
    vshade::asset::AssetManager assets;
    constexpr std::array sampleNames{
        "sample-1.flac",
        "sample-2.flac",
        "sample-3.wav",
    };
    for (const char* sampleName : sampleNames) {
        const auto handle = assets.load<vshade::audio::AudioClip>(
            testAssetDirectory / sampleName
        );
        const std::shared_ptr<vshade::audio::AudioClip> clip = assets.get(handle);

        REQUIRE(clip);
        CHECK(clip->sourcePath().filename() == sampleName);
        CHECK(clip->channels() > 0);
        CHECK(clip->sampleRate() > 0);
        CHECK(clip->frameCount() > 0);
        CHECK(clip->durationSeconds() > 0.0);
    }

    const vshade::asset::AudioLoader loader;
    CHECK_THROWS_AS(loader.load(""), std::invalid_argument);
    CHECK_THROWS_AS(
        loader.load("missing-audio-file.wav"),
        std::runtime_error
    );
}

TEST_CASE("Audio engine manages playback without opening an output device", "[audio]") {
    vshade::asset::AssetManager assets;
    const auto decodedHandle = assets.load<vshade::audio::AudioClip>(decodedSample);
    const auto streamedHandle = assets.load<vshade::audio::AudioClip>(streamedSample);
    const std::shared_ptr<vshade::audio::AudioClip> decodedClip =
        assets.get(decodedHandle);
    const std::shared_ptr<vshade::audio::AudioClip> streamedClip =
        assets.get(streamedHandle);
    vshade::audio::AudioEngine engine;

    CHECK_THROWS_AS(engine.setMasterVolume(1.0F), std::logic_error);
    engine.initialize({.enableDevice = false});
    CHECK_THROWS_AS(
        engine.initialize({.enableDevice = false}),
        std::logic_error
    );
    CHECK_THROWS_AS(engine.setMasterVolume(-1.0F), std::invalid_argument);
    CHECK_THROWS_AS(
        engine.setMasterVolume(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );

    engine.setMasterVolume(0.5F);
    auto voice = engine.play(decodedClip, {.looping = true});
    REQUIRE(voice.valid());
    CHECK(voice.state() == vshade::audio::AudioState::Playing);
    voice.setVolume(0.25F);
    voice.setPitch(1.1F);
    voice.setPosition({1.0F, 2.0F, 3.0F});
    voice.pause();
    CHECK(voice.state() == vshade::audio::AudioState::Paused);
    voice.resume();
    CHECK(voice.state() == vshade::audio::AudioState::Playing);
    voice.stop();
    CHECK(voice.state() == vshade::audio::AudioState::Stopped);
    engine.stop(decodedClip);
    engine.play(streamedClip, {
        .bus = vshade::audio::AudioBus::Music,
        .loadMode = vshade::audio::AudioLoadMode::Stream,
        .volume = 0.75F,
        .pitch = 1.0F,
        .looping = true,
    });
    engine.stop(streamedClip);
    engine.shutdown();
    engine.shutdown();
}

TEST_CASE("Scene audio is scoped while music survives scene changes", "[audio][scene-runtime]") {
    vshade::core::EngineServices services({.enableDevice = false});
    const auto clipReference = services.assets().reference<vshade::audio::AudioClip>(
        streamedSample
    );

    auto music = services.audio().playMusic(streamedSample, {.looping = true});
    REQUIRE(music.valid());

    vshade::scene::Scene scene("Audio scene");
    auto listener = scene.create("Listener");
    listener.add<vshade::scene::AudioListenerComponent>();
    auto source = scene.create("Emitter");
    auto& audioSource = source.add<vshade::scene::AudioSourceComponent>();
    audioSource.clipAsset = clipReference;
    audioSource.playOnStart = true;
    audioSource.looping = true;
    audioSource.spatial = true;

    vshade::scene::SceneRuntime runtime(services, {
        .physics2D = false,
        .physics3D = false,
        .rendering = false,
        .audio = true,
    });
    runtime.play(scene);
    REQUIRE(runtime.sceneAudio() != nullptr);
    CHECK(runtime.sceneAudio()->voiceCount() == 1);
    runtime.update(0.0F);
    runtime.stop();
    CHECK(music.state() == vshade::audio::AudioState::Playing);
    services.audio().stopMusic();
}
