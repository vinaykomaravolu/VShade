set(VSHADE_IMGUI_DIR "${PROJECT_SOURCE_DIR}/external/imgui")
set(VSHADE_IMGUIZMO_DIR "${PROJECT_SOURCE_DIR}/external/ImGuizmo")
set(VSHADE_IMGUI_FILE_DIALOG_DIR
    "${PROJECT_SOURCE_DIR}/external/ImGuiFileDialog"
)

find_package(OpenGL REQUIRED)

# Dear ImGui is an editor implementation detail. Keeping it in this target
# prevents ImGui headers and types from leaking into VShadeEngine's public API.
add_library(VShadeImGui STATIC
    "${VSHADE_IMGUI_DIR}/imgui.cpp"
    "${VSHADE_IMGUI_DIR}/imgui_demo.cpp"
    "${VSHADE_IMGUI_DIR}/imgui_draw.cpp"
    "${VSHADE_IMGUI_DIR}/imgui_tables.cpp"
    "${VSHADE_IMGUI_DIR}/imgui_widgets.cpp"
    "${VSHADE_IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${VSHADE_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
    "${VSHADE_IMGUIZMO_DIR}/src/ImGuizmo.cpp"
    "${VSHADE_IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.cpp"
)

target_include_directories(VShadeImGui SYSTEM PUBLIC
    "${VSHADE_IMGUI_DIR}"
    "${VSHADE_IMGUI_DIR}/backends"
    "${VSHADE_IMGUIZMO_DIR}/src"
    "${VSHADE_IMGUI_FILE_DIALOG_DIR}"
)
target_link_libraries(VShadeImGui PUBLIC glfw OpenGL::GL)
target_compile_features(VShadeImGui PUBLIC cxx_std_20)

add_library(VShadeEditorCore STATIC
    src/AssetImportService.cpp
    src/EditorDocument.cpp
    src/UndoHistory.cpp
)
add_library(VShade::EditorCore ALIAS VShadeEditorCore)
target_include_directories(VShadeEditorCore PUBLIC include)
target_link_libraries(VShadeEditorCore PUBLIC VShade::Engine)
target_compile_features(VShadeEditorCore PUBLIC cxx_std_20)

add_executable(VShadeEditor
    ImGui/ImGuiTheme.cpp
    src/main.cpp
    src/EditorApplication.cpp
    src/EditorCamera.cpp
    src/EditorIcons.cpp
    src/EditorLayer.cpp
    src/panels/Console.cpp
    src/panels/ContentBrowserPanel.cpp
    src/panels/InspectorPanel.cpp
    src/panels/SceneHierarchyPanel.cpp
    src/panels/Viewport.cpp
    src/widgets/AssetSelector.cpp
)
target_include_directories(VShadeEditor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    include
)
target_link_libraries(VShadeEditor PRIVATE
    VShade::EditorCore
    VShade::Engine
    VShadeImGui
)
target_compile_definitions(VShadeEditor PRIVATE
    VSHADE_EDITOR_ICON_DIR="$<TARGET_FILE_DIR:VShadeEditor>/icons"
)

set(VSHADE_EDITOR_ICONS
    assets/icons/folder.png
    assets/icons/file.png
    assets/icons/texture.png
    assets/icons/model.png
    assets/icons/audio.png
    assets/icons/scene.png
    assets/icons/prefab.png
    assets/icons/camera.png
    assets/icons/directional_light.png
    assets/icons/point_light.png
)
set_source_files_properties(${VSHADE_EDITOR_ICONS} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(VShadeEditor PRIVATE ${VSHADE_EDITOR_ICONS})

add_custom_command(TARGET VShadeEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:VShadeEditor>/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/folder.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/file.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/texture.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/model.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/audio.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/scene.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/prefab.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/camera.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/directional_light.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/point_light.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/speaker.png"
        "$<TARGET_FILE_DIR:VShadeEditor>/icons"
    COMMENT "Copying editor icons"
)

if(MSVC)
    target_compile_options(VShadeEditor PRIVATE /W4 /permissive-)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEditor PRIVATE /WX)
    endif()
else()
    target_compile_options(VShadeEditor PRIVATE -Wall -Wextra -Wpedantic)
    if(VSHADE_WARNINGS_AS_ERRORS)
        target_compile_options(VShadeEditor PRIVATE -Werror)
    endif()
endif()

set_target_properties(VShadeEditor PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:VShadeEditor>"
)
