#pragma once

#include "audio/AudioTypes.hpp"
#include "math/Vector.hpp"

#include <memory>

namespace vshade::audio {

class AudioEngine;

/** @brief Shared control handle for one playback instance. */
class AudioVoice final {
public:
    AudioVoice() = default;
    ~AudioVoice() = default;

    void pause();
    void resume();
    void stop() noexcept;
    void setVolume(float volume);
    void setPitch(float pitch);
    void setPosition(const math::Vec3& position);

    [[nodiscard]] AudioState state() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    explicit operator bool() const noexcept { return valid(); }

private:
    struct Impl;
    explicit AudioVoice(std::shared_ptr<Impl> impl) noexcept;

    std::shared_ptr<Impl> m_impl;
    friend class AudioEngine;
};

} // namespace vshade::audio
