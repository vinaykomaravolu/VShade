#include "core/Log.hpp"

#include <mutex>
#include <utility>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace vshade::core {
namespace {

std::mutex logger_mutex;
std::shared_ptr<spdlog::logger> engine_logger;
std::shared_ptr<spdlog::logger> game_logger;

void initializeLocked() {
    if (engine_logger && game_logger) {
        return;
    }

    engine_logger = spdlog::get("VShade");
    if (!engine_logger) {
        engine_logger = spdlog::stdout_color_mt("VShade");
    }

    game_logger = spdlog::get("Game");
    if (!game_logger) {
        game_logger = spdlog::stdout_color_mt("Game");
    }

    constexpr auto pattern = "%^[%T] [%n] [%l] %v%$";
    engine_logger->set_pattern(pattern);
    game_logger->set_pattern(pattern);

#if defined(NDEBUG)
    constexpr auto default_level = spdlog::level::info;
#else
    constexpr auto default_level = spdlog::level::trace;
#endif

    engine_logger->set_level(default_level);
    game_logger->set_level(default_level);
}

} // namespace

void Log::initialize() {
    const std::scoped_lock lock(logger_mutex);
    initializeLocked();
}

void Log::shutdown() {
    const std::scoped_lock lock(logger_mutex);

    // Only remove VShade-owned loggers. Do not shut down the host application's
    // entire spdlog registry.
    if (engine_logger) {
        engine_logger->flush();
        spdlog::drop(engine_logger->name());
        engine_logger.reset();
    }

    if (game_logger) {
        game_logger->flush();
        spdlog::drop(game_logger->name());
        game_logger.reset();
    }
}

void Log::setLevel(const spdlog::level::level_enum level) {
    const std::scoped_lock lock(logger_mutex);
    initializeLocked();
    engine_logger->set_level(level);
    game_logger->set_level(level);
}

std::shared_ptr<spdlog::logger> Log::engine() {
    const std::scoped_lock lock(logger_mutex);
    initializeLocked();
    return engine_logger;
}

std::shared_ptr<spdlog::logger> Log::game() {
    const std::scoped_lock lock(logger_mutex);
    initializeLocked();
    return game_logger;
}

} // namespace vshade::core
