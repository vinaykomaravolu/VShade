#include "panels/SceneHierarchyPanel.hpp"

#include <scene/Scene.hpp>
#include <scene/components/CoreComponents.hpp>

#include <utility>

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void SceneHierarchyPanel::setScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    m_scene = std::move(scene);
    m_selectedEntity = {};
}

void SceneHierarchyPanel::onImGuiRender() {
    ImGui::Begin("Hierarchy", nullptr, panelFlags);

    if (m_scene) {
        const auto entities = std::as_const(*m_scene).view<
            vshade::scene::UUIDComponent,
            vshade::scene::TagComponent
        >();

        for (const auto handle : entities) {
            const auto& uuid =
                entities.get<vshade::scene::UUIDComponent>(handle);
            const auto& tag =
                entities.get<vshade::scene::TagComponent>(handle);
            const vshade::scene::Entity entity =
                m_scene->findEntity(uuid.uuid);

            ImGui::PushID(static_cast<int>(entity.id()));
            if (ImGui::Selectable(
                    tag.tag.c_str(),
                    m_selectedEntity == entity
                )) {
                m_selectedEntity = entity;
            }
            ImGui::PopID();
        }
    }

    ImGui::End();
}

vshade::scene::Entity SceneHierarchyPanel::selectedEntity() const noexcept {
    if (!m_scene || !m_scene->valid(m_selectedEntity)) {
        return {};
    }
    return m_selectedEntity;
}

} // namespace editor
