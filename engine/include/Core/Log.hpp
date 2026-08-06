#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace VShade {

/** @brief Manages the engine and game spdlog logger instances. */
class Log final {
public:
    Log() = delete;

    /**
     * @brief Creates the engine and game loggers if they do not already exist.
     * @note Calling this is optional because first access initializes both loggers.
     */
    static void Initialize();

    /** @brief Flushes, unregisters, and releases both loggers. */
    static void Shutdown();

    /** @brief Sets the minimum severity emitted by both loggers. */
    static void SetLevel(spdlog::level::level_enum level);

    /** @brief Returns the logger reserved for engine implementation messages. */
    [[nodiscard]] static std::shared_ptr<spdlog::logger> Engine();

    /** @brief Returns the logger intended for game and sandbox messages. */
    [[nodiscard]] static std::shared_ptr<spdlog::logger> Game();
};

} // namespace VShade

/** @defgroup engine_logging Engine logging macros
 *  @brief Write messages through the engine logger.
 *  @{
 */
/** @brief Writes an engine trace message. */
#define ENGINE_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::Engine(), __VA_ARGS__)
/** @brief Writes an engine debug message. */
#define ENGINE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::Engine(), __VA_ARGS__)
/** @brief Writes an engine informational message. */
#define ENGINE_INFO(...) SPDLOG_LOGGER_INFO(::VShade::Log::Engine(), __VA_ARGS__)
/** @brief Writes an engine warning message. */
#define ENGINE_WARN(...) SPDLOG_LOGGER_WARN(::VShade::Log::Engine(), __VA_ARGS__)
/** @brief Writes an engine error message. */
#define ENGINE_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::Engine(), __VA_ARGS__)
/** @brief Writes an engine critical message. */
#define ENGINE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::Engine(), __VA_ARGS__)
/** @} */

/** @defgroup game_logging Game logging macros
 *  @brief Write messages through the game logger.
 *  @{
 */
/** @brief Writes a game trace message. */
#define GAME_TRACE(...) SPDLOG_LOGGER_TRACE(::VShade::Log::Game(), __VA_ARGS__)
/** @brief Writes a game debug message. */
#define GAME_DEBUG(...) SPDLOG_LOGGER_DEBUG(::VShade::Log::Game(), __VA_ARGS__)
/** @brief Writes a game informational message. */
#define GAME_INFO(...) SPDLOG_LOGGER_INFO(::VShade::Log::Game(), __VA_ARGS__)
/** @brief Writes a game warning message. */
#define GAME_WARN(...) SPDLOG_LOGGER_WARN(::VShade::Log::Game(), __VA_ARGS__)
/** @brief Writes a game error message. */
#define GAME_ERROR(...) SPDLOG_LOGGER_ERROR(::VShade::Log::Game(), __VA_ARGS__)
/** @brief Writes a game critical message. */
#define GAME_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::VShade::Log::Game(), __VA_ARGS__)
/** @} */
