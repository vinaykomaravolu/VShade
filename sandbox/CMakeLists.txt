set(VSHADE_SANDBOX_SHADERS
    shaders/shader.fs
    shaders/shader.vs
)

set(VSHADE_SANDBOX_TEXTURES
    textures/checkerboard.ppm
)

set(VSHADE_SANDBOX_ASSETS
    assets/ABeautifulGame.glb
)

add_executable(VShadeSandbox
    main.cpp
    ${VSHADE_SANDBOX_SHADERS}
    ${VSHADE_SANDBOX_TEXTURES}
    ${VSHADE_SANDBOX_ASSETS}
)
target_link_libraries(VShadeSandbox PRIVATE VShade::Engine)

if(MSVC)
    target_compile_options(VShadeSandbox PRIVATE /W4 /permissive-)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeSandbox PRIVATE /WX)
    endif()
else()
    target_compile_options(VShadeSandbox PRIVATE -Wall -Wextra -Wpedantic)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeSandbox PRIVATE -Werror)
    endif()
endif()
target_compile_definitions(VShadeSandbox PRIVATE
    VSHADE_SANDBOX_SHADER_DIR="$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
    VSHADE_SANDBOX_TEXTURE_DIR="$<TARGET_FILE_DIR:VShadeSandbox>/textures"
    VSHADE_SANDBOX_ASSET_DIR="$<TARGET_FILE_DIR:VShadeSandbox>/assets"
)

set_source_files_properties(
    ${VSHADE_SANDBOX_SHADERS}
    ${VSHADE_SANDBOX_TEXTURES}
    ${VSHADE_SANDBOX_ASSETS}
    PROPERTIES HEADER_FILE_ONLY TRUE
)

add_custom_command(TARGET VShadeSandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VShadeSandbox>/textures"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VShadeSandbox>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.fs"
        "$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/textures/checkerboard.ppm"
        "$<TARGET_FILE_DIR:VShadeSandbox>/textures"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/ABeautifulGame.glb"
        "$<TARGET_FILE_DIR:VShadeSandbox>/assets/ABeautifulGame.glb"
    COMMENT "Copying sandbox shaders, textures, and models"
)

set_target_properties(VShadeSandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:VShadeSandbox>"
)
