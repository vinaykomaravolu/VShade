#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::audio {
class AudioClip;
}

namespace vshade::asset {

/** @brief Uses miniaudio to validate an audio file and read its format metadata. */
class AudioLoader final : public AssetLoader<audio::AudioClip> {
public:
    AudioLoader() = default;
    ~AudioLoader() override = default;

    [[nodiscard]] std::shared_ptr<audio::AudioClip> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
