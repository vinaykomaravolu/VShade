set(VSHADE_IMGUI_DIR "${PROJECT_SOURCE_DIR}/external/imgui")
set(VSHADE_IMGUIZMO_DIR "${PROJECT_SOURCE_DIR}/external/ImGuizmo")

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
)

target_include_directories(VShadeImGui SYSTEM PUBLIC
    "${VSHADE_IMGUI_DIR}"
    "${VSHADE_IMGUI_DIR}/backends"
    "${VSHADE_IMGUIZMO_DIR}/src"
)
target_link_libraries(VShadeImGui PUBLIC glfw OpenGL::GL)
target_compile_features(VShadeImGui PUBLIC cxx_std_20)

add_executable(VShadeEditor
    src/main.cpp
    src/EditorApplication.cpp
    src/EditorCamera.cpp
    src/EditorLayer.cpp
    src/panels/Console.cpp
    src/panels/InspectorPanel.cpp
    src/panels/SceneHierarchyPanel.cpp
    src/panels/Viewport.cpp
)
target_include_directories(VShadeEditor PRIVATE include)
target_link_libraries(VShadeEditor PRIVATE VShade::Engine VShadeImGui)

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
