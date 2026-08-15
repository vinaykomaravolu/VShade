set(VSHADE_NUMBERED_EXAMPLES
    01_hello_window
    02_triangle
    03_input
    04_textured_quad
    05_camera_2d
    06_renderer_2d
    07_scene_entities
    08_sprite_scene
    09_scene_serialization
    10_framebuffer
    11_cube
    12_camera_3d
    13_lighting
    14_loading_model
    15_materials
    16_model_hierarchy
    17_physics_2d
    18_physics_collisions
    19_audio
    20_audio_3d
    21_asset_manager
    22_scripting
    23_simple_2d_game
    24_simple_3d_game
    25_rendering_workflows
)

set(VSHADE_FEATURE_EXAMPLES
    features/multiple_textures
    features/transparency
    features/multiple_lights
    features/normal_mapping
    features/render_to_texture
)

function(vshade_add_example example_path)
    string(REPLACE "/" "_" target_suffix "${example_path}")
    set(target "VShadeExample_${target_suffix}")
    add_executable(${target} "${example_path}/main.cpp")
    target_link_libraries(${target} PRIVATE VShade::Engine)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${target} PRIVATE
        VSHADE_EXAMPLE_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/output/${target_suffix}"
        VSHADE_EXAMPLE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${example_path}"
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
        FOLDER "Examples"
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

foreach(example IN LISTS VSHADE_NUMBERED_EXAMPLES VSHADE_FEATURE_EXAMPLES)
    vshade_add_example("${example}")
endforeach()

# Example 10 writes its readback to PNG without making stb part of VShade's
# public include surface.
target_include_directories(VShadeExample_10_framebuffer SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/external/stb"
)
