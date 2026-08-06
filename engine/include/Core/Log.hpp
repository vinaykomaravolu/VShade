#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace VShade {

class Log final {
public:
    Log() = delete;

    // Calling Initialize is optional; the first logger access initializes both.
    static void Initialize();
    static void Shutdown();
    static void SetLevel(spdlog::level::level_enum level);

    [[nodiscard]] static std::shared_ptr<spdlog::logger> Engine();
    [[nodiscard]] static std::shared_ptr<spdlog::logger> Game();
};

} // namespace VShade

#define ENGINE_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::Engine(), __VA_ARGS__)
#define ENGINE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::Engine(), __VA_ARGS__)
#define ENGINE_INFO(...) SPDLOG_LOGGER_INFO(::VShade::Log::Engine(), __VA_ARGS__)
#define ENGINE_WARN(...) SPDLOG_LOGGER_WARN(::VShade::Log::Engine(), __VA_ARGS__)
#define ENGINE_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::Engine(), __VA_ARGS__)
#define ENGINE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::Engine(), __VA_ARGS__)

#define GAME_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::Game(), __VA_ARGS__)
#define GAME_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::Game(), __VA_ARGS__)
#define GAME_INFO(...) SPDLOG_LOGGER_INFO(::VShade::Log::Game(), __VA_ARGS__)
#define GAME_WARN(...) SPDLOG_LOGGER_WARN(::VShade::Log::Game(), __VA_ARGS__)
#define GAME_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::Game(), __VA_ARGS__)
#define GAME_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::Game(), __VA_ARGS__)
