#include "panels/InspectorPanel.hpp"

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void InspectorPanel::onImGuiRender(const int selectedEntity) {
    ImGui::Begin("Inspector", nullptr, panelFlags);

    static constexpr const char* entityNames[] = {
        "Camera",
        "Player",
        "Cube",
        "Light",
    };

    if (selectedEntity >= 0 && selectedEntity < IM_ARRAYSIZE(entityNames)) {
        ImGui::TextUnformatted(entityNames[selectedEntity]);
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Position", m_position, 0.1F);
            ImGui::DragFloat3("Rotation", m_rotation, 0.5F);
            ImGui::DragFloat3("Scale", m_scale, 0.1F);
        }
    } else {
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::End();
}

} // namespace editor
