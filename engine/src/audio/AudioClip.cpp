#include "audio/AudioClip.hpp"

#include <utility>

namespace vshade::audio {

AudioClip::AudioClip(
    std::filesystem::path sourcePath,
    const std::uint32_t channels,
    const std::uint32_t sampleRate,
    const std::uint64_t frameCount
)
    : m_sourcePath(std::move(sourcePath).lexically_normal()),
      m_channels(channels),
      m_sampleRate(sampleRate),
      m_frameCount(frameCount) {}

const std::filesystem::path& AudioClip::sourcePath() const noexcept {
    return m_sourcePath;
}

std::uint32_t AudioClip::channels() const noexcept {
    return m_channels;
}

std::uint32_t AudioClip::sampleRate() const noexcept {
    return m_sampleRate;
}

std::uint64_t AudioClip::frameCount() const noexcept {
    return m_frameCount;
}

double AudioClip::durationSeconds() const noexcept {
    return m_sampleRate == 0
        ? 0.0
        : static_cast<double>(m_frameCount)
            / static_cast<double>(m_sampleRate);
}

} // namespace vshade::audio
