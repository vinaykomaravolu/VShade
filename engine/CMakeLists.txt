set(VSHADE_SOURCES
    src/audio/AudioClip.cpp
    src/audio/AudioEngine.cpp
    src/asset/AssetManager.cpp
    src/asset/AssetRegistry.cpp
    src/asset/loader/MeshLoader.cpp
    src/asset/loader/ModelLoader.cpp
    src/asset/loader/GltfLoaderUtils.cpp
    src/asset/loader/AudioLoader.cpp
    src/asset/loader/SceneLoader.cpp
    src/asset/loader/ShaderLoader.cpp
    src/asset/loader/TextureLoader.cpp
    src/core/Application.cpp
    src/core/Filesystem.cpp
    src/core/Log.cpp
    src/core/Time.cpp
    src/input/Input.cpp
    src/math/Matrix.cpp
    src/math/Quaternion.cpp
    src/math/Transform.cpp
    src/math/Vector.cpp
    src/platform/Window.cpp
    src/physics/box2d/PhysicsSystem2D.cpp
    src/physics/box2d/PhysicsWorld2D.cpp
    src/physics/jolt/PhysicsSystem3D.cpp
    src/physics/jolt/PhysicsWorld3D.cpp
    src/renderer/Buffer.cpp
    src/renderer/Camera.cpp
    src/renderer/CameraController.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Material.cpp
    src/renderer/MaterialParameters.cpp
    src/renderer/Mesh.cpp
    src/renderer/Model.cpp
    src/renderer/opengl/OpenGLUtils.cpp
    src/renderer/opengl/generated/src/gl.c
    src/renderer/Renderer.cpp
    src/renderer/Renderer2D.cpp
    src/renderer/Renderer3D.cpp
    src/renderer/RenderTypes.cpp
    src/renderer/Shader.cpp
    src/renderer/Texture.cpp
    src/renderer/VertexArray.cpp
    src/scene/Entity.cpp
    src/scene/Scene.cpp
    src/scene/SceneComponentRegistry.cpp
    src/scene/SceneSerializer.cpp
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

# spdlog appears in Log.hpp and GLM is part of the engine-facing math API.
# GLFW and the native OpenGL library remain implementation details.
target_link_libraries(VShadeEngine
    PUBLIC
        spdlog::spdlog
        glm::glm
        EnTT::EnTT
        nlohmann_json::nlohmann_json
        reflectcpp::reflectcpp
        fastgltf::fastgltf
    PRIVATE
        box2d::box2d
        Jolt
        glfw
        miniaudio
        miniaudio_common_options
        OpenGL::GL
)

# The logging macros are used by both the engine and game. Setting the active
# level publicly keeps trace/debug calls available in consumers too; runtime
# logger levels still decide which messages are printed.
target_compile_definitions(VShadeEngine PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# Apple marks its system OpenGL API deprecated. VShade intentionally supports
# that backend, so keep unrelated warnings-as-errors enabled while silencing
# only Apple's OpenGL deprecation annotations.
if(APPLE)
    target_compile_definitions(VShadeEngine PRIVATE GL_SILENCE_DEPRECATION)
endif()

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
