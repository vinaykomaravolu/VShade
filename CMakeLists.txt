cmake_minimum_required(VERSION 3.20)

project(
    VShade
    VERSION 0.1.0
    DESCRIPTION "A small cross-platform OpenGL game engine runtime"
    LANGUAGES CXX
)

include(CTest)

option(VSHADE_BUILD_SANDBOX "Build the example sandbox application" ON)
option(VSHADE_BUILD_DOCS "Build the VShade Doxygen API reference" OFF)
option(VSHADE_WARNINGS_AS_ERRORS "Treat VShade compiler warnings as errors" OFF)
option(VSHADE_ENABLE_COVERAGE "Instrument VShadeEngine for GCC coverage reports" OFF)

foreach(dependency IN ITEMS spdlog glfw glm)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${dependency}/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing dependency: external/${dependency}. "
            "Run: git submodule update --init --recursive"
        )
    endif()
endforeach()

if(BUILD_TESTING AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing dependency: external/Catch2. "
        "Run: git submodule update --init --recursive"
    )
endif()

# Keep dependency builds small and avoid installing their examples and tests.
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "" FORCE)

add_subdirectory(external/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)
add_subdirectory(external/glm EXCLUDE_FROM_ALL)

if(BUILD_TESTING)
    add_subdirectory(external/Catch2 EXCLUDE_FROM_ALL)
endif()

add_subdirectory(engine)

if(VSHADE_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(VSHADE_BUILD_DOCS)
    add_subdirectory(docs)
endif()
