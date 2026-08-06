#pragma once

#include "Core/Application.hpp"

#include <exception>
#include <iostream>
#include <type_traits>

#define SHADE_ENGINE_MAIN(application_type)                                      \
    int main() {                                                                \
        static_assert(                                                          \
            std::is_base_of_v<::VShade::Application, application_type>,         \
            "SHADE_ENGINE_MAIN requires a VShade::Application type"             \
        );                                                                      \
                                                                                \
        try {                                                                   \
            application_type application;                                      \
            return application.Run();                                           \
        } catch (const std::exception& error) {                                 \
            std::cerr << "Fatal error: " << error.what() << '\n';               \
            return 1;                                                           \
        } catch (...) {                                                         \
            std::cerr << "Fatal error: unknown exception\n";                    \
            return 1;                                                           \
        }                                                                       \
    }
