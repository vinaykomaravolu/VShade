set(VSHADE_SOURCES
    src/core/application.cpp
    src/core/filesystem.cpp
    src/core/log.cpp
    src/core/time.cpp
    src/input/input.cpp
    src/math/matrix.cpp
    src/math/quaternion.cpp
    src/math/transform.cpp
    src/math/vector.cpp
    src/platform/window.cpp
    src/renderer/buffer.cpp
    src/renderer/camera.cpp
    src/renderer/cameracontroller.cpp
    src/renderer/framebuffer.cpp
    src/renderer/material.cpp
    src/renderer/mesh.cpp
    src/renderer/opengl/openglutils.cpp
    src/renderer/opengl/generated/src/gl.c
    src/renderer/renderer.cpp
    src/renderer/renderer2d.cpp
    src/renderer/renderer3d.cpp
    src/renderer/rendertypes.cpp
    src/renderer/shader.cpp
    src/renderer/texture.cpp
    src/renderer/vertexarray.cpp
)

# The GLAD loader is generated from the pinned external/glad submodule and
# checked in so normal builds do not require Python or generator packages.

# STATIC means the engine is copied into each executable that links it.
add_library(VShadeEngine STATIC ${VSHADE_SOURCES})
add_library(VShade::Engine ALIAS VShadeEngine)

target_compile_features(VShadeEngine PUBLIC cxx_std_20)

target_include_directories(VShadeEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/opengl/generated/include
)

# stb_image is an implementation detail used by Texture2D::fromFile().
target_include_directories(VShadeEngine SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}/external/stb
)

find_package(OpenGL REQUIRED)

# spdlog appears in log.hpp and GLM is part of the engine-facing math API.
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
    target_compile_options(VShadeEngine PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/W4>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
        $<$<COMPILE_LANGUAGE:C>:/W0>
    )
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEngine PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/WX>)
    endif()
else()
    target_compile_options(VShadeEngine PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
        $<$<COMPILE_LANGUAGE:C>:-w>
    )
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEngine PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror>)
    endif()
endif()

set_target_properties(VShadeEngine PROPERTIES
    VERSION ${PROJECT_VERSION}
)
