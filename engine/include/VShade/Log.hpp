#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace VShade {

class Log final {
public:
    Log() = delete;

    // Calling initialize is optional; the first logger access initializes both.
    static void initialize();
    static void shutdown();
    static void set_level(spdlog::level::level_enum level);

    [[nodiscard]] static std::shared_ptr<spdlog::logger> engine();
    [[nodiscard]] static std::shared_ptr<spdlog::logger> client();
};

} // namespace VShade

#define VSHADE_ENGINE_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::engine(), __VA_ARGS__)
#define VSHADE_ENGINE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::engine(), __VA_ARGS__)
#define VSHADE_ENGINE_INFO(...)  SPDLOG_LOGGER_INFO(::VShade::Log::engine(), __VA_ARGS__)
#define VSHADE_ENGINE_WARN(...)  SPDLOG_LOGGER_WARN(::VShade::Log::engine(), __VA_ARGS__)
#define VSHADE_ENGINE_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::engine(), __VA_ARGS__)
#define VSHADE_ENGINE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::engine(), __VA_ARGS__)

#define VSHADE_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::client(), __VA_ARGS__)
#define VSHADE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::client(), __VA_ARGS__)
#define VSHADE_INFO(...)  SPDLOG_LOGGER_INFO(::VShade::Log::client(), __VA_ARGS__)
#define VSHADE_WARN(...)  SPDLOG_LOGGER_WARN(::VShade::Log::client(), __VA_ARGS__)
#define VSHADE_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::client(), __VA_ARGS__)
#define VSHADE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::client(), __VA_ARGS__)
