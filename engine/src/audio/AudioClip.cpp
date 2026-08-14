#include "audio/AudioClip.hpp"

#include <utility>

namespace vshade::audio {

AudioClip::AudioClip(
    std::filesystem::path sourcePath,
    const std::uint32_t channels,
    const std::uint32_t sampleRate
)
    : m_sourcePath(std::move(sourcePath).lexically_normal()),
      m_channels(channels),
      m_sampleRate(sampleRate) {}

const std::filesystem::path& AudioClip::sourcePath() const noexcept {
    return m_sourcePath;
}

std::uint32_t AudioClip::channels() const noexcept {
    return m_channels;
}

std::uint32_t AudioClip::sampleRate() const noexcept {
    return m_sampleRate;
}

} // namespace vshade::audio
