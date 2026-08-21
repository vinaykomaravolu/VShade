#pragma once

#include "asset/AssetHandle.hpp"

#include <filesystem>
#include <cstdint>

namespace vshade::asset {
class AudioLoader;
}

namespace vshade::audio {

/** @brief Validated audio asset metadata and its normalized source path. */
class AudioClip final {
public:
    /** @brief Returns the normalized source file used for playback. */
    [[nodiscard]] const std::filesystem::path& sourcePath() const noexcept;

    /** @brief Returns the number of interleaved audio channels. */
    [[nodiscard]] std::uint32_t channels() const noexcept;

    /** @brief Returns the number of PCM frames played per second. */
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;
    [[nodiscard]] std::uint64_t frameCount() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;

private:
    friend class asset::AudioLoader;
    friend class AudioEngine;

    AudioClip(
        std::filesystem::path sourcePath,
        std::uint32_t channels,
        std::uint32_t sampleRate,
        std::uint64_t frameCount
    );

    std::filesystem::path m_sourcePath;
    std::uint32_t m_channels = 0;
    std::uint32_t m_sampleRate = 0;
    std::uint64_t m_frameCount = 0;
};

/** @brief Stable asset reference to an audio clip. */
using AudioClipHandle = asset::AssetHandle<AudioClip>;

} // namespace vshade::audio
