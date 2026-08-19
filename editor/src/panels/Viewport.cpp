#include "panels/Viewport.hpp"

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void Viewport::onImGuiRender() {
    ImGui::Begin("Viewport", nullptr, panelFlags);

    const ImVec2 availableSize = ImGui::GetContentRegionAvail();
    const ImVec2 textSize = ImGui::CalcTextSize("Scene viewport");
    ImGui::SetCursorPos({
        (availableSize.x - textSize.x) * 0.5F,
        (availableSize.y - textSize.y) * 0.5F,
    });
    ImGui::TextDisabled("Scene viewport");

    ImGui::End();
}

} // namespace editor
