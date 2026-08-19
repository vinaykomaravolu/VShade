#include "panels/InspectorPanel.hpp"

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void InspectorPanel::onImGuiRender(
    const vshade::scene::Entity selectedEntity
) {
    ImGui::Begin("Inspector", nullptr, panelFlags);

    if (selectedEntity) {
        const auto name = selectedEntity.name();
        ImGui::TextUnformatted(name.data(), name.data() + name.size());
    } else {
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::End();
}

} // namespace editor
