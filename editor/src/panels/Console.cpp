#include "panels/Console.hpp"

#include <imgui.h>

namespace editor {

void Console::onImGuiRender() {
    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Console", nullptr, panelFlags);
    ImGui::TextDisabled("Editor console output will appear here.");
    ImGui::End();
}

} // namespace editor
