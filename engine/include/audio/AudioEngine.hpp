#pragma once

#include "audio/AudioClip.hpp"
#include "audio/AudioTypes.hpp"
#include "audio/AudioVoice.hpp"

#include <memory>

namespace vshade::audio {

/**
 * @brief Backend-independent interface for audio playback and mixing.
 *
 * The implementation owns all miniaudio state. No miniaudio types are exposed
 * through this public header.
 */
class AudioEngine final {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(AudioEngine&&) noexcept;
    AudioEngine& operator=(AudioEngine&&) noexcept;
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /** @brief Opens the default audio device and prepares the mixer. */
    void initialize(const AudioEngineConfig& config = {});

    /** @brief Stops playback and releases the audio device. */
    void shutdown() noexcept;

    /** @brief Sets the master mixer gain, where one is the original volume. */
    void setMasterVolume(float volume);

    /** @brief Starts a decoded or streamed playback instance of an audio clip. */
    AudioVoice play(
        std::shared_ptr<const AudioClip> clip,
        const AudioPlaybackSettings& settings = {}
    );

    /** @brief Stops every active playback instance of an audio clip. */
    void stop(const std::shared_ptr<const AudioClip>& clip);

    /** @brief Updates the single world-space listener used for spatial playback. */
    void setListenerTransform(
        const math::Vec3& position,
        const math::Vec3& forward,
        const math::Vec3& up
    );

    /** @brief Sets the gain of a logical mixer bus. */
    void setBusVolume(AudioBus bus, float volume);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::audio
