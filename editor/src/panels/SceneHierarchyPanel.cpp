#include "panels/SceneHierarchyPanel.hpp"

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void SceneHierarchyPanel::onImGuiRender() {
    ImGui::Begin("Hierarchy", nullptr, panelFlags);

    static constexpr const char* entities[] = {
        "Camera",
        "Player",
        "Cube",
        "Light",
    };

    for (int index = 0; index < IM_ARRAYSIZE(entities); ++index) {
        if (ImGui::Selectable(entities[index], m_selectedEntity == index)) {
            m_selectedEntity = index;
        }
    }

    ImGui::End();
}

int SceneHierarchyPanel::selectedEntity() const noexcept {
    return m_selectedEntity;
}

} // namespace editor
