#include "panels/SceneHierarchyPanel.hpp"

#include <input/Input.hpp>
#include <input/KeyCode.hpp>
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
}

void SceneHierarchyPanel::setReadOnly(const bool readOnly) noexcept {
    m_readOnly = readOnly;
}

void SceneHierarchyPanel::setSavePrefabHandler(
    std::function<void(vshade::scene::Entity)> handler
) {
    m_savePrefab = std::move(handler);
}

void SceneHierarchyPanel::onImGuiRender(
    vshade::scene::Entity& selectedEntity
) {
    ImGui::Begin("Hierarchy", nullptr, panelFlags);

    if (m_scene) {
        vshade::scene::Entity entityToDuplicate;
        vshade::scene::Entity entityToDelete;
        const auto entities = std::as_const(*m_scene).view<
            vshade::scene::UUIDComponent,
            vshade::scene::TagComponent
        >();

        for (const auto handle : entities) {
            const auto& uuid =
                entities.get<vshade::scene::UUIDComponent>(handle);
            const vshade::scene::Entity entity =
                m_scene->findEntity(uuid.uuid);

            switch (drawEntity(entity, selectedEntity)) {
                case EntityAction::Duplicate:
                    if (!m_readOnly) {
                        entityToDuplicate = entity;
                    }
                    break;
                case EntityAction::Delete:
                    if (!m_readOnly) {
                        entityToDelete = entity;
                    }
                    break;
                case EntityAction::None:
                    break;
            }
        }

        const bool hierarchyFocused = ImGui::IsWindowFocused(
            ImGuiFocusedFlags_RootAndChildWindows
        );
        const bool keyboardAvailable =
            !m_readOnly && hierarchyFocused && !ImGui::GetIO().WantTextInput;
        if (keyboardAvailable && m_scene->valid(selectedEntity)) {
            using vshade::input::Input;
            using vshade::input::KeyCode;

            const bool controlDown =
                Input::isKeyDown(KeyCode::LeftControl)
                || Input::isKeyDown(KeyCode::RightControl);
            if (controlDown && Input::isKeyPressed(KeyCode::D)) {
                entityToDuplicate = selectedEntity;
            }
            if (Input::isKeyPressed(KeyCode::Delete)) {
                entityToDelete = selectedEntity;
            }
        }

        if (!m_readOnly
            && ImGui::BeginPopupContextWindow(
                "HierarchyContext",
                ImGuiPopupFlags_MouseButtonRight
                    | ImGuiPopupFlags_NoOpenOverItems
            )) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                selectedEntity = m_scene->create("Entity");
            }
            ImGui::EndPopup();
        }

        if (m_scene->valid(entityToDuplicate)) {
            selectedEntity = m_scene->duplicateEntity(entityToDuplicate);
        }
        if (m_scene->valid(entityToDelete)) {
            if (selectedEntity == entityToDelete) {
                selectedEntity = {};
            }
            m_scene->destroyEntity(entityToDelete);
        }
    }

    ImGui::End();
}

SceneHierarchyPanel::EntityAction SceneHierarchyPanel::drawEntity(
    vshade::scene::Entity entity,
    vshade::scene::Entity& selectedEntity
) {
    ImGui::PushID(static_cast<int>(entity.id()));
    const auto& tag = entity.get<vshade::scene::TagComponent>();
    if (ImGui::Selectable(
            tag.tag.c_str(),
            selectedEntity == entity
        )) {
        selectedEntity = entity;
    }

    EntityAction action = EntityAction::None;
    if (!m_readOnly && ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Duplicate")) {
            action = EntityAction::Duplicate;
        }
        if (ImGui::MenuItem("Save as Prefab...") && m_savePrefab) {
            m_savePrefab(entity);
        }
        if (ImGui::MenuItem("Delete")) {
            action = EntityAction::Delete;
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return action;
}

} // namespace editor
