#include "core/time.hpp"

#include <chrono>

namespace vshade::core {
namespace {

using Clock = std::chrono::steady_clock;

Clock::time_point start_time = Clock::now();
Clock::time_point previous_frame_time = start_time;
float delta_time = 0.0F;
double elapsed_time = 0.0;
std::uint64_t frame_count = 0;

} // namespace

float Time::DeltaTime() noexcept {
    return delta_time;
}

double Time::ElapsedTime() noexcept {
    return elapsed_time;
}

std::uint64_t Time::FrameCount() noexcept {
    return frame_count;
}

void Time::Reset() noexcept {
    start_time = Clock::now();
    previous_frame_time = start_time;
    delta_time = 0.0F;
    elapsed_time = 0.0;
    frame_count = 0;
}

void Time::Tick() noexcept {
    const auto now = Clock::now();
    delta_time = std::chrono::duration<float>(now - previous_frame_time).count();
    elapsed_time = std::chrono::duration<double>(now - start_time).count();
    previous_frame_time = now;
    ++frame_count;
}

} // namespace vshade::core
