#pragma once

#include "core/application.hpp"

#include <exception>
#include <iostream>
#include <type_traits>

/**
 * @def SHADE_ENGINE_MAIN(application_type)
 * @brief Defines `main()` for a concrete vshade::core::Application type.
 * @param application_type Default-constructible type derived from vshade::core::Application.
 *
 * The generated entry point runs the application and reports uncaught
 * exceptions to standard error before returning a failure exit code.
 */
#define SHADE_ENGINE_MAIN(application_type)                                      \
    int main() {                                                                \
        static_assert(                                                          \
            std::is_base_of_v<::vshade::core::Application, application_type>,   \
            "SHADE_ENGINE_MAIN requires a vshade::core::Application type"       \
        );                                                                      \
                                                                                \
        try {                                                                   \
            application_type application;                                      \
            return application.run();                                           \
        } catch (const std::exception& error) {                                 \
            std::cerr << "Fatal error: " << error.what() << '\n';               \
            return 1;                                                           \
        } catch (...) {                                                         \
            std::cerr << "Fatal error: unknown exception\n";                    \
            return 1;                                                           \
        }                                                                       \
    }
