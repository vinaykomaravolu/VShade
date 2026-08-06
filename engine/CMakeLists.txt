set(VSHADE_SOURCES
    src/Core/Application.cpp
    src/Core/FileSystem.cpp
    src/Core/Log.cpp
    src/Core/Time.cpp
    src/Platform/Window.cpp
)

# STATIC means the engine is copied into each executable that links it.
add_library(VShadeEngine STATIC ${VSHADE_SOURCES})
add_library(VShade::Engine ALIAS VShadeEngine)

target_compile_features(VShadeEngine PUBLIC cxx_std_20)

target_include_directories(VShadeEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(OpenGL REQUIRED)

# spdlog appears in Log.hpp and GLM is part of the engine-facing math API.
# GLFW and the native OpenGL library remain implementation details.
target_link_libraries(VShadeEngine
    PUBLIC
        spdlog::spdlog
        glm::glm
    PRIVATE
        glfw
        OpenGL::GL
)

# The logging macros are used by both the engine and game. Setting the active
# level publicly keeps trace/debug calls available in consumers too; runtime
# logger levels still decide which messages are printed.
target_compile_definitions(VShadeEngine PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

if(VSHADE_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "VSHADE_ENABLE_COVERAGE currently requires GCC")
    endif()

    # Instrument only engine implementation files. The INTERFACE link option
    # supplies the gcov runtime when a test executable links this static library.
    target_compile_options(VShadeEngine PRIVATE
        --coverage
        -fprofile-abs-path
        -O0
        -g
    )
    target_link_options(VShadeEngine INTERFACE --coverage)
endif()

if(MSVC)
    target_compile_options(VShadeEngine PRIVATE /W4 /permissive-)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEngine PRIVATE /WX)
    endif()
else()
    target_compile_options(VShadeEngine PRIVATE -Wall -Wextra -Wpedantic)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEngine PRIVATE -Werror)
    endif()
endif()

set_target_properties(VShadeEngine PROPERTIES
    VERSION ${PROJECT_VERSION}
)
