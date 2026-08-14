#include <asset/AssetManager.hpp>
#include <asset/loader/AudioLoader.hpp>
#include <audio/AudioClip.hpp>
#include <audio/AudioEngine.hpp>
#include <audio/AudioTypes.hpp>
#include <scene/Components.hpp>

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
    engine.play(decodedClip);
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
