add_executable(VShadeSandbox main.cpp)
target_link_libraries(VShadeSandbox PRIVATE VShade::Engine)

set_target_properties(VShadeSandbox PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
