#pragma once

#include <cstdint>

namespace vshade::core {

class Application;

/** @brief Provides read-only timing information for the current engine run. */
class Time final {
public:
    Time() = delete;

    /** @brief Returns the duration of the most recently started frame in seconds. */
    [[nodiscard]] static float DeltaTime() noexcept;

    /** @brief Returns seconds elapsed since the application main loop started. */
    [[nodiscard]] static double ElapsedTime() noexcept;

    /** @brief Returns the number of frames started during this application run. */
    [[nodiscard]] static std::uint64_t FrameCount() noexcept;

private:
    friend class Application;

    /** @brief Resets all timing values for a new application run. */
    static void Reset() noexcept;

    /** @brief Advances the timing values at the start of a frame. */
    static void Tick() noexcept;
};

} // namespace vshade::core
