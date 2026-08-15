#pragma once

#include "audio/AudioEngine.hpp"

#include <filesystem>
#include <memory>

namespace vshade::asset { class AssetManager; }

namespace vshade::audio {

/** @brief Game-facing audio facade backed by application-lifetime assets and mixing. */
class AudioService final {
public:
    AudioService(AudioEngine& engine, asset::AssetManager& assets) noexcept;

    AudioVoice play(
        std::shared_ptr<const AudioClip> clip,
        const AudioPlaybackSettings& settings = {}
    );
    AudioVoice play(
        const std::filesystem::path& path,
        const AudioPlaybackSettings& settings = {}
    );
    AudioVoice playOneShot(
        const std::filesystem::path& path,
        AudioPlaybackSettings settings = {}
    );
    AudioVoice playMusic(
        const std::filesystem::path& path,
        AudioPlaybackSettings settings = {}
    );
    void stopMusic() noexcept;

    void stop(const std::shared_ptr<const AudioClip>& clip);
    void setMasterVolume(float volume);
    void setBusVolume(AudioBus bus, float volume);
    void setListenerTransform(
        const math::Vec3& position,
        const math::Vec3& forward,
        const math::Vec3& up
    );

    /** @brief Direct access for advanced backend-level audio operations. */
    [[nodiscard]] AudioEngine& engine() noexcept;

private:
    AudioEngine* m_engine = nullptr;
    asset::AssetManager* m_assets = nullptr;
    AudioVoice m_music;
};

} // namespace vshade::audio
