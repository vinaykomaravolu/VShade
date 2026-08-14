#include "audio/AudioEngine.hpp"

#include <miniaudio.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vshade::audio {
namespace {

[[nodiscard]] std::runtime_error audioError(
    const std::string& action,
    const ma_result result
) {
    return std::runtime_error(action + ": " + ma_result_description(result));
}

void validateVolume(const float volume) {
    if (!std::isfinite(volume) || volume < 0.0F) {
        throw std::invalid_argument("Audio volume must be finite and non-negative");
    }
}

void validatePitch(const float pitch) {
    if (!std::isfinite(pitch) || pitch <= 0.0F) {
        throw std::invalid_argument("Audio pitch must be finite and positive");
    }
}

struct PlayingSound final {
    std::shared_ptr<const AudioClip> clip;
    ma_sound sound{};
    bool initialized = false;

    ~PlayingSound() {
        if (initialized) {
            ma_sound_uninit(&sound);
        }
    }

    PlayingSound() = default;
    PlayingSound(const PlayingSound&) = delete;
    PlayingSound& operator=(const PlayingSound&) = delete;
    PlayingSound(PlayingSound&&) = delete;
    PlayingSound& operator=(PlayingSound&&) = delete;
};

} // namespace

struct AudioEngine::Impl {
    ma_engine engine{};
    ma_sound_group musicGroup{};
    ma_sound_group sfxGroup{};
    std::vector<std::unique_ptr<PlayingSound>> sounds;
    bool engineInitialized = false;
    bool musicGroupInitialized = false;
    bool sfxGroupInitialized = false;

    ~Impl() {
        shutdown();
    }

    void shutdown() noexcept {
        sounds.clear();
        if (sfxGroupInitialized) {
            ma_sound_group_uninit(&sfxGroup);
            sfxGroupInitialized = false;
        }
        if (musicGroupInitialized) {
            ma_sound_group_uninit(&musicGroup);
            musicGroupInitialized = false;
        }
        if (engineInitialized) {
            ma_engine_uninit(&engine);
            engineInitialized = false;
        }
    }

    void removeFinishedSounds() {
        std::erase_if(
            sounds,
            [](const std::unique_ptr<PlayingSound>& sound) {
                return ma_sound_at_end(&sound->sound) == MA_TRUE;
            }
        );
    }

    [[nodiscard]] ma_sound_group* group(const AudioBus bus) noexcept {
        switch (bus) {
        case AudioBus::Master: return nullptr;
        case AudioBus::Music: return &musicGroup;
        case AudioBus::SFX: return &sfxGroup;
        }
        return nullptr;
    }
};

AudioEngine::AudioEngine()
    : m_impl(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() = default;
AudioEngine::AudioEngine(AudioEngine&&) noexcept = default;
AudioEngine& AudioEngine::operator=(AudioEngine&&) noexcept = default;

void AudioEngine::initialize(const AudioEngineConfig& config) {
    if (!m_impl) {
        throw std::logic_error("A moved-from audio engine cannot be initialized");
    }
    if (m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is already initialized");
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.noDevice = config.enableDevice ? MA_FALSE : MA_TRUE;
    if (!config.enableDevice) {
        engineConfig.channels = 2;
        engineConfig.sampleRate = 48'000;
    }
    ma_result result = ma_engine_init(&engineConfig, &m_impl->engine);
    if (result != MA_SUCCESS) {
        throw audioError("Failed to initialize the audio engine", result);
    }
    m_impl->engineInitialized = true;

    result = ma_sound_group_init(&m_impl->engine, 0, nullptr, &m_impl->musicGroup);
    if (result != MA_SUCCESS) {
        m_impl->shutdown();
        throw audioError("Failed to initialize the music bus", result);
    }
    m_impl->musicGroupInitialized = true;

    result = ma_sound_group_init(&m_impl->engine, 0, nullptr, &m_impl->sfxGroup);
    if (result != MA_SUCCESS) {
        m_impl->shutdown();
        throw audioError("Failed to initialize the SFX bus", result);
    }
    m_impl->sfxGroupInitialized = true;
}

void AudioEngine::shutdown() noexcept {
    if (m_impl) {
        m_impl->shutdown();
    }
}

void AudioEngine::setMasterVolume(const float volume) {
    validateVolume(volume);
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    const ma_result result = ma_engine_set_volume(&m_impl->engine, volume);
    if (result != MA_SUCCESS) {
        throw audioError("Failed to set the master volume", result);
    }
}

void AudioEngine::play(
    std::shared_ptr<const AudioClip> clip,
    const AudioPlaybackSettings& settings
) {
    validateVolume(settings.volume);
    validatePitch(settings.pitch);
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    if (!clip) {
        throw std::invalid_argument("An audio clip cannot be null");
    }

    m_impl->removeFinishedSounds();
    auto sound = std::make_unique<PlayingSound>();
    sound->clip = std::move(clip);

    const ma_uint32 flags = settings.loadMode == AudioLoadMode::Stream
        ? MA_SOUND_FLAG_STREAM
        : MA_SOUND_FLAG_DECODE;
#if defined(_WIN32)
    ma_result result = ma_sound_init_from_file_w(
        &m_impl->engine,
        sound->clip->sourcePath().c_str(),
        flags,
        m_impl->group(settings.bus),
        nullptr,
        &sound->sound
    );
#else
    const std::string path = sound->clip->sourcePath().string();
    ma_result result = ma_sound_init_from_file(
        &m_impl->engine,
        path.c_str(),
        flags,
        m_impl->group(settings.bus),
        nullptr,
        &sound->sound
    );
#endif
    if (result != MA_SUCCESS) {
        throw audioError(
            "Failed to create playback for " + sound->clip->sourcePath().string(),
            result
        );
    }
    sound->initialized = true;
    ma_sound_set_volume(&sound->sound, settings.volume);
    ma_sound_set_pitch(&sound->sound, settings.pitch);
    ma_sound_set_looping(&sound->sound, settings.looping ? MA_TRUE : MA_FALSE);

    const ma_result startResult = ma_sound_start(&sound->sound);
    if (startResult != MA_SUCCESS) {
        throw audioError("Failed to start audio playback", startResult);
    }
    m_impl->sounds.push_back(std::move(sound));
}

void AudioEngine::stop(const std::shared_ptr<const AudioClip>& clip) {
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    if (!clip) {
        throw std::invalid_argument("An audio clip cannot be null");
    }
    const std::filesystem::path& path = clip->sourcePath();
    std::erase_if(
        m_impl->sounds,
        [&path](const std::unique_ptr<PlayingSound>& sound) {
            if (sound->clip->sourcePath() != path) {
                return ma_sound_at_end(&sound->sound) == MA_TRUE;
            }
            static_cast<void>(ma_sound_stop(&sound->sound));
            return true;
        }
    );
}

} // namespace vshade::audio
