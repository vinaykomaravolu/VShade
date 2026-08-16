#pragma once

#include "math/Vector.hpp"

namespace vshade::audio {

/** @brief Controls how the audio backend is initialized. */
struct AudioEngineConfig {
    /** @brief Opens the default output device when true. Disable for offline tests. */
    bool enableDevice = true;
};

/** @brief Logical mixer channel used to group related sounds. */
enum class AudioBus {
    Master,
    Music,
    SFX,
};

/** @brief Distance curve used when attenuating a spatial sound. */
enum class AttenuationModel {
    None,
    Inverse,
    Linear,
    Exponential,
};

/** @brief Current playback state of an audio source. */
enum class AudioState {
    Stopped,
    Playing,
    Paused,
};

/** @brief Selects whether a playback instance is resident or streamed. */
enum class AudioLoadMode {
    /** @brief Decodes the complete clip for low-latency repeated playback. */
    Decode,
    /** @brief Decodes incrementally to keep long music and ambience out of memory. */
    Stream,
};

/** @brief Settings applied to one playback instance. */
struct AudioPlaybackSettings {
    AudioBus bus = AudioBus::SFX;
    AudioLoadMode loadMode = AudioLoadMode::Decode;
    float volume = 1.0F;
    float pitch = 1.0F;
    bool looping = false;
    bool spatial = false;
    math::Vec3 position{0.0F};
    AttenuationModel attenuation = AttenuationModel::Inverse;
    float minimumDistance = 1.0F;
    float maximumDistance = 100.0F;
};

} // namespace vshade::audio
