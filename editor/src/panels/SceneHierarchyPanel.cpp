#include "panels/SceneHierarchyPanel.hpp"

#include <core/Log.hpp>
#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <scene/Scene.hpp>
#include <scene/components/CoreComponents.hpp>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

constexpr const char* entityDragDropType = "VShadeEntity";

void sortEntities(std::vector<vshade::scene::Entity>& entities) {
    std::ranges::sort(
        entities,
        [](const vshade::scene::Entity& left,
           const vshade::scene::Entity& right) {
            const std::string_view leftName = left.name();
            const std::string_view rightName = right.name();
            if (leftName != rightName) {
                return leftName < rightName;
            }
            return left.uuid() < right.uuid();
        }
    );
}

[[nodiscard]] std::vector<vshade::scene::Entity> sortedChildren(
    vshade::scene::Scene& scene,
    const vshade::scene::Entity parent
) {
    std::vector<vshade::scene::Entity> children = scene.children(parent);
    sortEntities(children);
    return children;
}

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
        vshade::scene::Entity entityToUnparent;
        vshade::scene::Entity entityToCreateChild;

        std::vector<vshade::scene::Entity> roots;
        const auto entities = std::as_const(*m_scene).view<
            vshade::scene::UUIDComponent,
            vshade::scene::TagComponent
        >();
        for (const auto handle : entities) {
            const auto& uuid =
                entities.get<vshade::scene::UUIDComponent>(handle);
            const vshade::scene::Entity entity =
                m_scene->findEntity(uuid.uuid);
            if (entity && !m_scene->parent(entity)) {
                roots.push_back(entity);
            }
        }
        sortEntities(roots);

        for (const vshade::scene::Entity entity : roots) {
            const EntityCommand command = drawEntity(entity, selectedEntity);
            switch (command.action) {
                case EntityAction::Duplicate:
                    entityToDuplicate = command.entity;
                    break;
                case EntityAction::Delete:
                    entityToDelete = command.entity;
                    break;
                case EntityAction::Unparent:
                    entityToUnparent = command.entity;
                    break;
                case EntityAction::CreateChild:
                    entityToCreateChild = command.entity;
                    break;
                case EntityAction::None:
                    break;
            }
        }

        const ImVec2 leftover = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton(
            "##HierarchyEmpty",
            {
                leftover.x > 1.0F ? leftover.x : 1.0F,
                leftover.y > 24.0F ? leftover.y : 24.0F,
            }
        );
        if (ImGui::IsItemClicked()) {
            selectedEntity = {};
        }
        if (!m_readOnly && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(entityDragDropType)) {
                const auto uuid =
                    *static_cast<const std::uint64_t*>(payload->Data);
                const vshade::scene::Entity child = m_scene->findEntity(uuid);
                if (m_scene->valid(child) && m_scene->parent(child)) {
                    m_scene->clearParent(child);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (!m_readOnly && ImGui::BeginPopupContextItem("HierarchyEmptyContext")) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                selectedEntity = m_scene->create("Entity");
            }
            ImGui::EndPopup();
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

        if (!m_readOnly && m_scene->valid(entityToCreateChild)) {
            selectedEntity = m_scene->create("Entity");
            m_scene->setParent(selectedEntity, entityToCreateChild);
        }
        if (!m_readOnly && m_scene->valid(entityToUnparent)) {
            m_scene->clearParent(entityToUnparent);
        }
        if (!m_readOnly && m_scene->valid(entityToDuplicate)) {
            selectedEntity = m_scene->duplicateEntity(entityToDuplicate);
        }
        if (!m_readOnly && m_scene->valid(entityToDelete)) {
            if (selectedEntity == entityToDelete) {
                selectedEntity = {};
            }
            m_scene->destroyEntity(entityToDelete);
        }
    }

    ImGui::End();
}

SceneHierarchyPanel::EntityCommand SceneHierarchyPanel::drawEntity(
    vshade::scene::Entity entity,
    vshade::scene::Entity& selectedEntity
) {
    const std::vector<vshade::scene::Entity> children =
        sortedChildren(*m_scene, entity);
    const bool hasChildren = !children.empty();
    const auto& tag = entity.get<vshade::scene::TagComponent>();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding
        | ImGuiTreeNodeFlags_DefaultOpen;
    if (selectedEntity == entity) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    const auto* const nodeId = reinterpret_cast<void*>(entity.uuid());
    const bool opened = ImGui::TreeNodeEx(
        nodeId,
        flags,
        "%s",
        tag.tag.c_str()
    );
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedEntity = entity;
    }

    if (!m_readOnly && ImGui::BeginDragDropSource()) {
        const std::uint64_t uuid = entity.uuid();
        ImGui::SetDragDropPayload(
            entityDragDropType,
            &uuid,
            sizeof(uuid)
        );
        ImGui::TextUnformatted(tag.tag.c_str());
        ImGui::EndDragDropSource();
    }

    if (!m_readOnly && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(entityDragDropType)) {
            const auto uuid = *static_cast<const std::uint64_t*>(payload->Data);
            const vshade::scene::Entity child = m_scene->findEntity(uuid);
            if (m_scene->valid(child)) {
                try {
                    m_scene->setParent(child, entity);
                } catch (const std::exception& error) {
                    ENGINE_WARN(
                        "Could not parent '{}': {}",
                        std::string(child.name()),
                        error.what()
                    );
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    EntityCommand command;
    if (!m_readOnly && ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Create Child Entity")) {
            command = {EntityAction::CreateChild, entity};
        }
        if (ImGui::MenuItem("Duplicate")) {
            command = {EntityAction::Duplicate, entity};
        }
        if (ImGui::MenuItem("Save as Prefab...") && m_savePrefab) {
            m_savePrefab(entity);
        }
        if (m_scene->parent(entity)
            && ImGui::MenuItem("Unparent")) {
            command = {EntityAction::Unparent, entity};
        }
        if (ImGui::MenuItem("Delete")) {
            command = {EntityAction::Delete, entity};
        }
        ImGui::EndPopup();
    }

    if (opened && hasChildren) {
        for (const vshade::scene::Entity child : children) {
            const EntityCommand childCommand = drawEntity(child, selectedEntity);
            if (childCommand.action != EntityAction::None) {
                command = childCommand;
            }
        }
        ImGui::TreePop();
    }

    return command;
}

} // namespace editor
