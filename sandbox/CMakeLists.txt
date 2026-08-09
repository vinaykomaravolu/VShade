set(VSHADE_SANDBOX_SHADERS
    shaders/shader.fs
    shaders/shader.vs
)

add_executable(VShadeSandbox
    main.cpp
    ${VSHADE_SANDBOX_SHADERS}
)
target_link_libraries(VShadeSandbox PRIVATE VShade::Engine)
target_compile_definitions(VShadeSandbox PRIVATE
    VSHADE_SANDBOX_SHADER_DIR="$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
)

set_source_files_properties(${VSHADE_SANDBOX_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_command(TARGET VShadeSandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vs"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.fs"
        "$<TARGET_FILE_DIR:VShadeSandbox>/shaders"
    COMMENT "Copying sandbox shaders"
)

set_target_properties(VShadeSandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:VShadeSandbox>"
)
