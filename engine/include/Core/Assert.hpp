#pragma once

#include "Core/Log.hpp"

#include <cassert>

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
