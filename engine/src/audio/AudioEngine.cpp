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

[[nodiscard]] ma_attenuation_model nativeAttenuation(const AttenuationModel model) {
    switch (model) {
        case AttenuationModel::None: return ma_attenuation_model_none;
        case AttenuationModel::Inverse: return ma_attenuation_model_inverse;
        case AttenuationModel::Linear: return ma_attenuation_model_linear;
        case AttenuationModel::Exponential: return ma_attenuation_model_exponential;
    }
    return ma_attenuation_model_inverse;
}

} // namespace

struct AudioVoice::Impl final {
    std::shared_ptr<const AudioClip> clip;
    ma_sound sound{};
    AudioState playbackState = AudioState::Stopped;
    bool initialized = false;

    ~Impl() { uninitialize(); }

    void uninitialize() noexcept {
        if (initialized) {
            static_cast<void>(ma_sound_stop(&sound));
            ma_sound_uninit(&sound);
            initialized = false;
        }
        playbackState = AudioState::Stopped;
    }
};

struct AudioEngine::Impl {
    ma_engine engine{};
    ma_sound_group musicGroup{};
    ma_sound_group sfxGroup{};
    std::vector<std::shared_ptr<AudioVoice::Impl>> sounds;
    bool engineInitialized = false;
    bool musicGroupInitialized = false;
    bool sfxGroupInitialized = false;

    ~Impl() {
        shutdown();
    }

    void shutdown() noexcept {
        for (const auto& sound : sounds) {
            sound->uninitialize();
        }
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
            [](const std::shared_ptr<AudioVoice::Impl>& sound) {
                if (!sound->initialized || ma_sound_at_end(&sound->sound) == MA_TRUE) {
                    sound->uninitialize();
                    return true;
                }
                return false;
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

AudioVoice AudioEngine::play(
    std::shared_ptr<const AudioClip> clip,
    const AudioPlaybackSettings& settings
) {
    validateVolume(settings.volume);
    validatePitch(settings.pitch);
    if (!std::isfinite(settings.minimumDistance) || settings.minimumDistance < 0.0F ||
        !std::isfinite(settings.maximumDistance) ||
        settings.maximumDistance <= settings.minimumDistance ||
        !std::isfinite(settings.position.x) || !std::isfinite(settings.position.y) ||
        !std::isfinite(settings.position.z)) {
        throw std::invalid_argument("Audio spatial playback settings are invalid");
    }
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    if (!clip) {
        throw std::invalid_argument("An audio clip cannot be null");
    }

    m_impl->removeFinishedSounds();
    auto sound = std::make_shared<AudioVoice::Impl>();
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
    ma_sound_set_spatialization_enabled(
        &sound->sound,
        settings.spatial ? MA_TRUE : MA_FALSE
    );
    ma_sound_set_position(
        &sound->sound,
        settings.position.x,
        settings.position.y,
        settings.position.z
    );
    ma_sound_set_attenuation_model(
        &sound->sound,
        nativeAttenuation(settings.attenuation)
    );
    ma_sound_set_min_distance(&sound->sound, settings.minimumDistance);
    ma_sound_set_max_distance(&sound->sound, settings.maximumDistance);

    const ma_result startResult = ma_sound_start(&sound->sound);
    if (startResult != MA_SUCCESS) {
        throw audioError("Failed to start audio playback", startResult);
    }
    sound->playbackState = AudioState::Playing;
    m_impl->sounds.push_back(sound);
    return AudioVoice(std::move(sound));
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
        [&path](const std::shared_ptr<AudioVoice::Impl>& sound) {
            if (sound->clip->sourcePath() != path) {
                if (!sound->initialized || ma_sound_at_end(&sound->sound) == MA_TRUE) {
                    sound->uninitialize();
                    return true;
                }
                return false;
            }
            sound->uninitialize();
            return true;
        }
    );
}

void AudioEngine::setListenerTransform(
    const math::Vec3& position,
    const math::Vec3& forward,
    const math::Vec3& up
) {
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    const auto finite = [](const math::Vec3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!finite(position) || !finite(forward) || !finite(up)) {
        throw std::invalid_argument("Audio listener transform must be finite");
    }
    ma_engine_listener_set_position(&m_impl->engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&m_impl->engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&m_impl->engine, 0, up.x, up.y, up.z);
}

void AudioEngine::setBusVolume(const AudioBus bus, const float volume) {
    validateVolume(volume);
    if (!m_impl || !m_impl->engineInitialized) {
        throw std::logic_error("The audio engine is not initialized");
    }
    if (bus == AudioBus::Master) {
        setMasterVolume(volume);
        return;
    }
    ma_sound_group_set_volume(m_impl->group(bus), volume);
}

AudioVoice::AudioVoice(std::shared_ptr<Impl> impl) noexcept
    : m_impl(std::move(impl)) {}

void AudioVoice::pause() {
    if (!valid()) {
        throw std::logic_error("Cannot pause an invalid audio voice");
    }
    const ma_result result = ma_sound_stop(&m_impl->sound);
    if (result != MA_SUCCESS) {
        throw audioError("Failed to pause audio voice", result);
    }
    m_impl->playbackState = AudioState::Paused;
}

void AudioVoice::resume() {
    if (!valid()) {
        throw std::logic_error("Cannot resume an invalid audio voice");
    }
    const ma_result result = ma_sound_start(&m_impl->sound);
    if (result != MA_SUCCESS) {
        throw audioError("Failed to resume audio voice", result);
    }
    m_impl->playbackState = AudioState::Playing;
}

void AudioVoice::stop() noexcept {
    if (m_impl) {
        m_impl->uninitialize();
    }
}

void AudioVoice::setVolume(const float volume) {
    validateVolume(volume);
    if (!valid()) {
        throw std::logic_error("Cannot update an invalid audio voice");
    }
    ma_sound_set_volume(&m_impl->sound, volume);
}

void AudioVoice::setPitch(const float pitch) {
    validatePitch(pitch);
    if (!valid()) {
        throw std::logic_error("Cannot update an invalid audio voice");
    }
    ma_sound_set_pitch(&m_impl->sound, pitch);
}

void AudioVoice::setPosition(const math::Vec3& position) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z)) {
        throw std::invalid_argument("Audio voice position must be finite");
    }
    if (!valid()) {
        throw std::logic_error("Cannot update an invalid audio voice");
    }
    ma_sound_set_position(&m_impl->sound, position.x, position.y, position.z);
}

AudioState AudioVoice::state() const noexcept {
    if (!valid()) {
        return AudioState::Stopped;
    }
    if (ma_sound_at_end(&m_impl->sound) == MA_TRUE) {
        return AudioState::Stopped;
    }
    return m_impl->playbackState;
}

bool AudioVoice::valid() const noexcept {
    return m_impl && m_impl->initialized;
}

} // namespace vshade::audio
