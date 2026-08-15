#include "audio/AudioService.hpp"

#include "asset/AssetManager.hpp"

namespace vshade::audio {

AudioService::AudioService(
    AudioEngine& engine,
    asset::AssetManager& assets
) noexcept : m_engine(&engine), m_assets(&assets) {}

AudioVoice AudioService::play(
    std::shared_ptr<const AudioClip> clip,
    const AudioPlaybackSettings& settings
) {
    return m_engine->play(std::move(clip), settings);
}

AudioVoice AudioService::play(
    const std::filesystem::path& path,
    const AudioPlaybackSettings& settings
) {
    return play(m_assets->loadResource<AudioClip>(path).shared(), settings);
}

AudioVoice AudioService::playOneShot(
    const std::filesystem::path& path,
    AudioPlaybackSettings settings
) {
    settings.bus = AudioBus::SFX;
    settings.looping = false;
    return play(path, settings);
}

AudioVoice AudioService::playMusic(
    const std::filesystem::path& path,
    AudioPlaybackSettings settings
) {
    settings.bus = AudioBus::Music;
    settings.loadMode = AudioLoadMode::Stream;
    m_music.stop();
    m_music = play(path, settings);
    return m_music;
}

void AudioService::stopMusic() noexcept {
    m_music.stop();
    m_music = {};
}

void AudioService::stop(const std::shared_ptr<const AudioClip>& clip) {
    m_engine->stop(clip);
}

void AudioService::setMasterVolume(const float volume) {
    m_engine->setMasterVolume(volume);
}

void AudioService::setBusVolume(const AudioBus bus, const float volume) {
    m_engine->setBusVolume(bus, volume);
}

void AudioService::setListenerTransform(
    const math::Vec3& position,
    const math::Vec3& forward,
    const math::Vec3& up
) {
    m_engine->setListenerTransform(position, forward, up);
}

AudioEngine& AudioService::engine() noexcept { return *m_engine; }

} // namespace vshade::audio
