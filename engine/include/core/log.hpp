#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace vshade::core {

/** @brief Manages the engine and game spdlog logger instances. */
class Log final {
public:
    Log() = delete;

    /**
     * @brief Creates the engine and game loggers if they do not already exist.
     * @note Calling this is optional because first access initializes both loggers.
     */
    static void initialize();

    /** @brief Flushes, unregisters, and releases both loggers. */
    static void shutdown();

    /**
     * @brief Sets the minimum severity emitted by both loggers.
     * @param level Lowest severity that should be emitted.
     */
    static void setLevel(spdlog::level::level_enum level);

    /**
     * @brief Returns the logger reserved for engine implementation messages.
     * @return Shared engine logger, initialized on first access when necessary.
     */
    [[nodiscard]] static std::shared_ptr<spdlog::logger> engine();

    /**
     * @brief Returns the logger intended for game and sandbox messages.
     * @return Shared game logger, initialized on first access when necessary.
     */
    [[nodiscard]] static std::shared_ptr<spdlog::logger> game();
};

} // namespace vshade::core

/** @defgroup engine_logging Engine logging macros
 *  @brief Write messages through the engine logger.
 *  @{
 */
/**
 * @brief Writes an engine trace message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_TRACE(...) SPDLOG_LOGGER_TRACE(::vshade::core::Log::engine(), __VA_ARGS__)
/**
 * @brief Writes an engine debug message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_DEBUG(...) SPDLOG_LOGGER_DEBUG(::vshade::core::Log::engine(), __VA_ARGS__)
/**
 * @brief Writes an engine informational message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_INFO(...) SPDLOG_LOGGER_INFO(::vshade::core::Log::engine(), __VA_ARGS__)
/**
 * @brief Writes an engine warning message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_WARN(...) SPDLOG_LOGGER_WARN(::vshade::core::Log::engine(), __VA_ARGS__)
/**
 * @brief Writes an engine error message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_ERROR(...) SPDLOG_LOGGER_ERROR(::vshade::core::Log::engine(), __VA_ARGS__)
/**
 * @brief Writes an engine critical message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define ENGINE_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::vshade::core::Log::engine(), __VA_ARGS__)
/** @} */

/** @defgroup game_logging Game logging macros
 *  @brief Write messages through the game logger.
 *  @{
 */
/**
 * @brief Writes a game trace message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_TRACE(...) SPDLOG_LOGGER_TRACE(::vshade::core::Log::game(), __VA_ARGS__)
/**
 * @brief Writes a game debug message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_DEBUG(...) SPDLOG_LOGGER_DEBUG(::vshade::core::Log::game(), __VA_ARGS__)
/**
 * @brief Writes a game informational message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_INFO(...) SPDLOG_LOGGER_INFO(::vshade::core::Log::game(), __VA_ARGS__)
/**
 * @brief Writes a game warning message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_WARN(...) SPDLOG_LOGGER_WARN(::vshade::core::Log::game(), __VA_ARGS__)
/**
 * @brief Writes a game error message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_ERROR(...) SPDLOG_LOGGER_ERROR(::vshade::core::Log::game(), __VA_ARGS__)
/**
 * @brief Writes a game critical message.
 * @param ... spdlog-compatible format string and arguments.
 */
#define GAME_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::vshade::core::Log::game(), __VA_ARGS__)
/** @} */
