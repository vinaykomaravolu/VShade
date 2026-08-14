add_executable(VShadeTests
    application_tests.cpp
    cameracontroller_tests.cpp
    filesystem_tests.cpp
    log_tests.cpp
    material_tests.cpp
    material_visual_tests.cpp
    math_tests.cpp
    renderer2d_tests.cpp
    renderer2d_visual_tests.cpp
    renderer3d_tests.cpp
    renderer3d_visual_tests.cpp
    renderer_tests.cpp
    renderer_visual_tests.cpp
    scene_tests.cpp
    visual/imagecomparison.cpp
)

# Tests consume the same public engine target as a real game. Catch2 supplies
# main(), so the test sources only need to declare TEST_CASE blocks.
target_link_libraries(VShadeTests
    PRIVATE
        VShade::Engine
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
    VSHADE_MATERIAL_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/material"
    VSHADE_RENDER_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render"
    VSHADE_RENDER2D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render2d"
    VSHADE_RENDER3D_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/render3d"
    VSHADE_SCENE_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/output/scene"
    VSHADE_SANDBOX_SHADER_DIR="${PROJECT_SOURCE_DIR}/sandbox/shaders"
    VSHADE_SANDBOX_TEXTURE_DIR="${PROJECT_SOURCE_DIR}/sandbox/textures"
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
