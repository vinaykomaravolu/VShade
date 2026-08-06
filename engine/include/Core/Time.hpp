#pragma once

#include <cstdint>

namespace VShade {

class Application;

class Time final {
public:
    Time() = delete;

    [[nodiscard]] static float DeltaTime() noexcept;
    [[nodiscard]] static double ElapsedTime() noexcept;
    [[nodiscard]] static std::uint64_t FrameCount() noexcept;

private:
    friend class Application;

    static void Reset() noexcept;
    static void Tick() noexcept;
};

} // namespace VShade
