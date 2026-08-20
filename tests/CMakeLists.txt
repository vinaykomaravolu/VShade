add_executable(VShadeTests
    ApplicationTests.cpp
    AssetTests.cpp
    AudioTests.cpp
    CameraControllerTests.cpp
    DebugDrawTests.cpp
    DebugDrawVisualTests.cpp
    EditorTests.cpp
    FilesystemTests.cpp
    LightingTests.cpp
    LogTests.cpp
    MaterialTests.cpp
    MaterialVisualTests.cpp
    MathTests.cpp
    Physics2DTests.cpp
    Physics2DVisualTests.cpp
    Physics3DTests.cpp
    Physics3DVisualTests.cpp
    ProjectTests.cpp
    Renderer2DTests.cpp
    Renderer2DVisualTests.cpp
    Renderer3DTests.cpp
    Renderer3DVisualTests.cpp
    RendererTests.cpp
    RendererVisualTests.cpp
    SceneTests.cpp
    SceneLightingTests.cpp
    ScriptingTests.cpp
    ScriptingVisualTests.cpp
    visual/ImageComparison.cpp
)

# Tests consume the same public engine target as a real game. Catch2 supplies
# main(), so the test sources only need to declare TEST_CASE blocks.
target_link_libraries(VShadeTests
    PRIVATE
        VShade::Engine
        VShade::EditorCore
        Catch2::Catch2WithMain
)

target_compile_features(VShadeTests PRIVATE cxx_std_20)

target_include_directories(VShadeTests PRIVATE
    ${PROJECT_SOURCE_DIR}/external/stb
)

target_compile_definitions(VShadeTests PRIVATE
    VSHADE_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
    VSHADE_TEST_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
    VSHADE_FILESYSTEM_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/filesystem"
    VSHADE_EDITOR_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/editor"
    VSHADE_MATERIAL_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/material"
    VSHADE_PHYSICS2D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/physics2d"
    VSHADE_PHYSICS3D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/physics3d"
    VSHADE_PROJECT_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/project"
    VSHADE_RENDER_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render"
    VSHADE_RENDER2D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render2d"
    VSHADE_RENDER3D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render3d"
    VSHADE_SCENE_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/scene"
    VSHADE_SANDBOX_SHADER_DIR="${PROJECT_SOURCE_DIR}/sandbox/shaders"
    VSHADE_SANDBOX_TEXTURE_DIR="${PROJECT_SOURCE_DIR}/sandbox/textures"
    VSHADE_SANDBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/sandbox/assets"
)

if(MSVC)
    target_compile_options(VShadeTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(VShadeTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Register each Catch2 TEST_CASE as an individual CTest test. This gives useful
# names and lets CTest run one case at a time when a failure is investigated.
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/external/Catch2/extras")
include(Catch)
catch_discover_tests(VShadeTests ADD_TAGS_AS_LABELS)
