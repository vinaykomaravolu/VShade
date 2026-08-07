#pragma once

#include "core/log.hpp"

#include <cassert>

/**
 * @def ENGINE_ASSERT(condition, ...)
 * @brief Reports an engine programming error and stops a debug build.
 * @param condition Expression that must evaluate to true.
 * @param ... spdlog-compatible message and optional formatting arguments.
 * @note Compiles to a no-op when `NDEBUG` is defined.
 */
/**
 * @def GAME_ASSERT(condition, ...)
 * @brief Reports a game programming error and stops a debug build.
 * @param condition Expression that must evaluate to true.
 * @param ... spdlog-compatible message and optional formatting arguments.
 * @note Compiles to a no-op when `NDEBUG` is defined.
 */
#if defined(NDEBUG)
    #define ENGINE_ASSERT(condition, ...) ((void)0)
    #define GAME_ASSERT(condition, ...) ((void)0)
#else
    #define ENGINE_ASSERT(condition, ...)                                      \
        do {                                                                   \
            if (!(condition)) {                                                \
                ENGINE_CRITICAL("Assertion failed: {}", #condition);           \
                ENGINE_CRITICAL(__VA_ARGS__);                                  \
                assert(condition);                                             \
            }                                                                  \
        } while (false)

    #define GAME_ASSERT(condition, ...)                                        \
        do {                                                                   \
            if (!(condition)) {                                                \
                GAME_CRITICAL("Assertion failed: {}", #condition);             \
                GAME_CRITICAL(__VA_ARGS__);                                    \
                assert(condition);                                             \
            }                                                                  \
        } while (false)
#endif
