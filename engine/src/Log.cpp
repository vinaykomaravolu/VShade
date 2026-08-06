#include "VShade/Log.hpp"

#include <mutex>
#include <utility>

#include <spdlog/sinks/stdout_color_sinks.h>

namespace VShade {
namespace {

std::mutex logger_mutex;
std::shared_ptr<spdlog::logger> engine_logger;
std::shared_ptr<spdlog::logger> client_logger;

void initialize_locked() {
    if (engine_logger && client_logger) {
        return;
    }

    engine_logger = spdlog::get("VShade");
    if (!engine_logger) {
        engine_logger = spdlog::stdout_color_mt("VShade");
    }

    client_logger = spdlog::get("Application");
    if (!client_logger) {
        client_logger = spdlog::stdout_color_mt("Application");
    }

    constexpr auto pattern = "%^[%T] [%n] [%l] %v%$";
    engine_logger->set_pattern(pattern);
    client_logger->set_pattern(pattern);

#if defined(NDEBUG)
    constexpr auto default_level = spdlog::level::info;
#else
    constexpr auto default_level = spdlog::level::trace;
#endif

    engine_logger->set_level(default_level);
    client_logger->set_level(default_level);
}

} // namespace

void Log::initialize() {
    const std::scoped_lock lock(logger_mutex);
    initialize_locked();
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

    if (client_logger) {
        client_logger->flush();
        spdlog::drop(client_logger->name());
        client_logger.reset();
    }
}

void Log::set_level(const spdlog::level::level_enum level) {
    const std::scoped_lock lock(logger_mutex);
    initialize_locked();
    engine_logger->set_level(level);
    client_logger->set_level(level);
}

std::shared_ptr<spdlog::logger> Log::engine() {
    const std::scoped_lock lock(logger_mutex);
    initialize_locked();
    return engine_logger;
}

std::shared_ptr<spdlog::logger> Log::client() {
    const std::scoped_lock lock(logger_mutex);
    initialize_locked();
    return client_logger;
}

} // namespace VShade
