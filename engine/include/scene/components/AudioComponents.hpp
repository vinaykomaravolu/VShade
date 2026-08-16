#pragma once

#include "audio/AudioClip.hpp"
#include "audio/AudioTypes.hpp"
#include "asset/AssetReference.hpp"

namespace vshade::scene {

/** @brief Serializable playback settings for sound emitted by an entity. */
struct AudioSourceComponent {
    audio::AudioClipHandle clip;
    audio::AudioBus bus = audio::AudioBus::SFX;
    audio::AudioLoadMode loadMode = audio::AudioLoadMode::Decode;
    float volume = 1.0F;
    float pitch = 1.0F;
    bool looping = false;
    bool playOnStart = false;
    bool spatial = false;
    /** @brief Preferred resolvable reference; clip remains a legacy ID fallback. */
    asset::AssetReference<audio::AudioClip> clipAsset;
    audio::AttenuationModel attenuation = audio::AttenuationModel::Inverse;
    float minimumDistance = 1.0F;
    float maximumDistance = 100.0F;
};

/** @brief Marks an entity transform as an audio listener. */
struct AudioListenerComponent {
    bool active = true;
};

} // namespace vshade::scene
