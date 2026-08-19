cmake_minimum_required(VERSION 3.23)

project(
    VShade
    VERSION 0.1.0
    DESCRIPTION "A small cross-platform OpenGL game engine runtime"
    LANGUAGES C CXX
)

include(CTest)

option(VSHADE_BUILD_SANDBOX "Build the example sandbox application" ON)
option(VSHADE_BUILD_EDITOR "Build the VShade editor" ON)
option(VSHADE_BUILD_EXAMPLES "Build standalone learning examples" OFF)
option(VSHADE_BUILD_DOCS "Build the VShade Doxygen API reference" OFF)
option(VSHADE_WARNINGS_AS_ERRORS "Treat VShade compiler warnings as errors" OFF)
option(VSHADE_ENABLE_COVERAGE "Instrument VShadeEngine for GCC coverage reports" OFF)

foreach(dependency IN ITEMS spdlog glfw glm simdjson fastgltf box2d)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${dependency}/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing dependency: external/${dependency}. "
            "Run: git submodule update --init --recursive"
        )
    endif()
endforeach()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/jolt/Build/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing dependency: external/jolt. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/entt/src/entt/entt.hpp")
    message(FATAL_ERROR
        "Missing dependency: external/entt. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include/nlohmann/json.hpp")
    message(FATAL_ERROR
        "Missing dependency: external/json. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/reflect-cpp/include/rfl/json.hpp")
    message(FATAL_ERROR
        "Missing dependency: external/reflect-cpp. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(BUILD_TESTING AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/Catch2/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing dependency: external/Catch2. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/stb/stb_image.h")
    message(FATAL_ERROR
        "Missing dependency: external/stb. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio/miniaudio.h")
    message(FATAL_ERROR
        "Missing dependency: external/miniaudio. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(VSHADE_BUILD_EDITOR AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui.cpp")
    message(FATAL_ERROR
        "Missing dependency: external/imgui. "
        "Run: git submodule update --init --recursive"
    )
endif()

if(VSHADE_BUILD_EDITOR AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ImGuizmo/src/ImGuizmo.cpp")
    message(FATAL_ERROR
        "Missing dependency: external/ImGuizmo. "
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
set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX2D_DOCS OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_COMPILE_WARNING_AS_ERROR OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_INSTALL OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_EXTRA_NODES ON CACHE BOOL "" FORCE)

# Jolt is a private engine backend. Keep its subproject from replacing VShade's
# compiler flags/runtime selection or building optional tooling and GPU backends.
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(JPH_USE_CPU_COMPUTE OFF CACHE BOOL "" FORCE)

add_subdirectory(external/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)
add_subdirectory(external/glm EXCLUDE_FROM_ALL)
add_subdirectory(external/simdjson EXCLUDE_FROM_ALL)
add_subdirectory(external/fastgltf EXCLUDE_FROM_ALL)
add_subdirectory(external/box2d EXCLUDE_FROM_ALL)
add_subdirectory(external/miniaudio EXCLUDE_FROM_ALL)
add_subdirectory(external/jolt/Build external/jolt-build EXCLUDE_FROM_ALL)

# reflect-cpp supplies automatic C++20 field reflection for game components.
# Only its JSON backend is enabled; bundled dependencies keep configuration
# self-contained and avoid downloading packages during normal builds.
set(REFLECTCPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_CHECK_HEADERS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_ALL_FORMATS OFF CACHE BOOL "" FORCE)
set(REFLECTCPP_JSON ON CACHE BOOL "" FORCE)
set(REFLECTCPP_USE_BUNDLED_DEPENDENCIES ON CACHE BOOL "" FORCE)
set(REFLECTCPP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/reflect-cpp EXCLUDE_FROM_ALL)
target_include_directories(reflectcpp SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/reflect-cpp/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/reflect-cpp/include/rfl/thirdparty"
)

# EnTT is header-only. Defining its interface target avoids configuring EnTT's
# development-only targets and keeps the dependency surface small.
add_library(VShadeEnTT INTERFACE)
add_library(EnTT::EnTT ALIAS VShadeEnTT)
target_include_directories(VShadeEnTT SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/entt/src"
)

# JSON backs SceneSerializer and the public custom-component registration API.
# Avoid its tests/install rules by exposing only its header directory.
add_library(VShadeJson INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS VShadeJson)
target_include_directories(VShadeJson SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include"
)

if(BUILD_TESTING)
    add_subdirectory(external/Catch2 EXCLUDE_FROM_ALL)
endif()

add_subdirectory(engine)

if(VSHADE_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

if(VSHADE_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()

if(VSHADE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(VSHADE_BUILD_DOCS)
    add_subdirectory(docs)
endif()
