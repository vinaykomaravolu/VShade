set(VSHADE_SOURCES
    src/Log.cpp
    src/Window.cpp
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
