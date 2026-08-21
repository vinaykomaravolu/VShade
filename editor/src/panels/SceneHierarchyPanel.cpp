#include "panels/SceneHierarchyPanel.hpp"

#include "EditorIcons.hpp"
#include "ImGui/ImGuiTheme.hpp"

#include <core/Log.hpp>
#include <renderer/Lighting.hpp>
#include <renderer/Texture.hpp>
#include <scene/Scene.hpp>
#include <scene/components/AudioComponents.hpp>
#include <scene/components/CoreComponents.hpp>
#include <scene/components/LightComponent.hpp>
#include <scene/components/PrefabInstanceComponent.hpp>
#include <scene/components/RenderComponents.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoCollapse;
constexpr const char* entityDragDropType = "VShadeEntity";

[[nodiscard]] std::string lower(std::string_view text) {
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](const unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return result;
}

[[nodiscard]] std::int64_t siblingOrder(
    const vshade::scene::Entity entity
) {
    const auto* order =
        entity.tryGet<vshade::scene::HierarchyOrderComponent>();
    return order
        ? order->siblingOrder
        : static_cast<std::int64_t>(entity.uuid() & 0x7FFFFFFFFFFFFFFFULL);
}

void sortBySiblingOrder(std::vector<vshade::scene::Entity>& entities) {
    std::ranges::sort(
        entities,
        [](const auto left, const auto right) {
            const std::int64_t leftOrder = siblingOrder(left);
            const std::int64_t rightOrder = siblingOrder(right);
            return leftOrder != rightOrder
                ? leftOrder < rightOrder
                : left.uuid() < right.uuid();
        }
    );
}

[[nodiscard]] bool prefabMember(
    const vshade::scene::Scene& scene,
    const vshade::scene::Entity entity
) {
    if (entity.has<vshade::scene::PrefabInstanceComponent>()) {
        return true;
    }
    const auto instances = scene.view<
        const vshade::scene::PrefabInstanceComponent>();
    for (const auto [handle, instance] : instances.each()) {
        (void)handle;
        if (std::ranges::any_of(instance.entities, [entity](const auto& link) {
                return link.instanceUuid == entity.uuid();
            })) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::shared_ptr<vshade::renderer::Texture2D> entityIcon(
    const vshade::scene::Entity entity
) {
    if (entity.has<vshade::scene::PrefabInstanceComponent>()) {
        return EditorIcons::prefab();
    }
    if (entity.has<vshade::scene::CameraComponent>()) {
        return EditorIcons::camera();
    }
    if (const auto* light = entity.tryGet<vshade::scene::LightComponent>()) {
        return std::holds_alternative<vshade::renderer::DirectionalLight>(
            light->light
        ) ? EditorIcons::directionalLight() : EditorIcons::pointLight();
    }
    if (entity.has<vshade::scene::AudioSourceComponent>()) {
        return EditorIcons::speaker();
    }
    if (entity.has<vshade::scene::ModelRendererComponent>()) {
        return EditorIcons::model();
    }
    if (entity.has<vshade::scene::SpriteRendererComponent>()) {
        return EditorIcons::texture();
    }
    return {};
}

} // namespace

void SceneHierarchyPanel::setScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    m_scene = std::move(scene);
    m_selectedUuids.clear();
    m_pendingDeleteUuids.clear();
    m_primarySelectionUuid = 0;
    m_renamingUuid = 0;
}

void SceneHierarchyPanel::setReadOnly(const bool readOnly) noexcept {
    m_readOnly = readOnly;
}

void SceneHierarchyPanel::setSavePrefabHandler(
    std::function<void(vshade::scene::Entity)> handler
) {
    m_savePrefab = std::move(handler);
}

void SceneHierarchyPanel::setEditHooks(SceneEditHooks hooks) {
    m_editHooks = std::move(hooks);
}

std::vector<vshade::scene::Entity> SceneHierarchyPanel::orderedRoots() const {
    std::vector<vshade::scene::Entity> roots;
    if (!m_scene) {
        return roots;
    }
    const auto entities = std::as_const(*m_scene).view<
        vshade::scene::UUIDComponent,
        vshade::scene::TagComponent>();
    for (const auto handle : entities) {
        const auto& uuid = entities.get<vshade::scene::UUIDComponent>(handle);
        const vshade::scene::Entity entity = m_scene->findEntity(uuid.uuid);
        if (entity && !m_scene->parent(entity)) {
            roots.push_back(entity);
        }
    }
    sortBySiblingOrder(roots);
    return roots;
}

std::vector<vshade::scene::Entity> SceneHierarchyPanel::orderedChildren(
    const vshade::scene::Entity parent
) const {
    std::vector<vshade::scene::Entity> children = m_scene->children(parent);
    sortBySiblingOrder(children);
    return children;
}

bool SceneHierarchyPanel::matchesFilter(
    const vshade::scene::Entity entity
) const {
    if (m_searchBuffer[0] == '\0') {
        return true;
    }
    const std::string query = lower(m_searchBuffer.data());
    if (lower(entity.name()).find(query) != std::string::npos) {
        return true;
    }
    return std::ranges::any_of(
        orderedChildren(entity),
        [this](const auto child) { return matchesFilter(child); }
    );
}

void SceneHierarchyPanel::beginRename(const vshade::scene::Entity entity) {
    if (m_readOnly || !m_scene || !m_scene->valid(entity)) {
        return;
    }
    if (const auto* state =
            entity.tryGet<vshade::scene::HierarchyStateComponent>();
        state && state->locked) {
        return;
    }
    m_renameBuffer.fill('\0');
    const std::string_view name = entity.name();
    const std::size_t count = std::min(
        name.size(),
        m_renameBuffer.size() - 1
    );
    std::memcpy(m_renameBuffer.data(), name.data(), count);
    m_renamingUuid = entity.uuid();
    m_focusRename = true;
    m_renameValidationError = false;
}

std::vector<vshade::scene::Entity> SceneHierarchyPanel::commandEntities(
    const vshade::scene::Entity contextEntity
) const {
    std::vector<vshade::scene::Entity> result;
    if (!m_scene || !m_scene->valid(contextEntity)) {
        return result;
    }
    if (!m_selectedUuids.contains(contextEntity.uuid())) {
        result.push_back(contextEntity);
        return result;
    }
    for (const std::uint64_t uuid : m_selectedUuids) {
        const auto entity = m_scene->findEntity(uuid);
        if (entity) {
            result.push_back(entity);
        }
    }
    sortBySiblingOrder(result);
    return result;
}

std::vector<vshade::scene::Entity> SceneHierarchyPanel::subtree(
    const vshade::scene::Entity root
) const {
    std::vector<vshade::scene::Entity> result{root};
    for (const auto child : orderedChildren(root)) {
        auto descendants = subtree(child);
        result.insert(result.end(), descendants.begin(), descendants.end());
    }
    return result;
}

void SceneHierarchyPanel::appendSiblingOrder(
    vshade::scene::Entity entity
) {
    const auto parent = m_scene->parent(entity);
    auto siblings = parent ? orderedChildren(parent) : orderedRoots();
    std::int64_t maximum = -1024;
    for (const auto sibling : siblings) {
        if (sibling != entity) {
            maximum = std::max(maximum, siblingOrder(sibling));
        }
    }
    entity.set<vshade::scene::HierarchyOrderComponent>(maximum + 1024);
}

void SceneHierarchyPanel::moveSibling(
    vshade::scene::Entity entity,
    const int direction
) {
    const auto parent = m_scene->parent(entity);
    auto siblings = parent ? orderedChildren(parent) : orderedRoots();
    const auto found = std::ranges::find(siblings, entity);
    if (found == siblings.end()) {
        return;
    }
    const std::ptrdiff_t index = std::distance(siblings.begin(), found);
    const std::ptrdiff_t destination = index + direction;
    if (destination < 0
        || destination >= static_cast<std::ptrdiff_t>(siblings.size())) {
        return;
    }
    std::swap(siblings[index], siblings[destination]);
    for (std::size_t sibling = 0; sibling < siblings.size(); ++sibling) {
        siblings[sibling].set<vshade::scene::HierarchyOrderComponent>(
            static_cast<std::int64_t>(sibling) * 1024
        );
    }
}

void SceneHierarchyPanel::drawDeleteConfirmation(
    vshade::scene::Entity& selectedEntity
) {
    if (m_openDeleteConfirmation) {
        ImGui::OpenPopup("Delete Entities?");
        m_openDeleteConfirmation = false;
    }
    if (!ImGui::BeginPopupModal(
            "Delete Entities?",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
        return;
    }

    std::unordered_set<std::uint64_t> affected;
    bool includesPrefab = false;
    for (const std::uint64_t uuid : m_pendingDeleteUuids) {
        const auto entity = m_scene ? m_scene->findEntity(uuid)
                                    : vshade::scene::Entity{};
        if (!entity) {
            continue;
        }
        includesPrefab |= prefabMember(*m_scene, entity);
        for (const auto descendant : subtree(entity)) {
            affected.insert(descendant.uuid());
        }
    }

    ImGui::Text(
        "Delete %zu selected object%s and %zu total object%s?",
        m_pendingDeleteUuids.size(),
        m_pendingDeleteUuids.size() == 1 ? "" : "s",
        affected.size(),
        affected.size() == 1 ? "" : "s"
    );
    ImGui::TextDisabled("Children in each selected subtree will also be deleted.");
    if (includesPrefab) {
        ImGui::TextColored(
            ui::color(ui::ColorRole::Warning),
            "This selection includes a linked prefab boundary."
        );
    }

    ui::pushDestructiveButtonStyle();
    const bool confirm = ImGui::Button("Delete", ui::scaled(96.0F, 0.0F));
    ui::popDestructiveButtonStyle();
    ImGui::SameLine();
    const bool cancel = ImGui::Button("Cancel", ui::scaled(96.0F, 0.0F));

    if (confirm) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        std::vector<vshade::scene::Entity> entities;
        entities.reserve(affected.size());
        for (const std::uint64_t uuid : affected) {
            const auto entity = m_scene->findEntity(uuid);
            if (entity) {
                entities.push_back(entity);
            }
        }
        const auto depth = [this](vshade::scene::Entity entity) {
            std::size_t value = 0;
            while (entity) {
                entity = m_scene->parent(entity);
                if (entity) {
                    ++value;
                }
            }
            return value;
        };
        std::ranges::sort(entities, [&depth](const auto left, const auto right) {
            return depth(left) > depth(right);
        });
        for (const auto entity : entities) {
            if (m_scene->valid(entity)) {
                m_scene->destroyEntity(entity);
            }
        }
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
        if (selectedEntity && affected.contains(selectedEntity.uuid())) {
            selectedEntity = {};
        }
        for (const auto uuid : affected) {
            m_selectedUuids.erase(uuid);
        }
        m_primarySelectionUuid = selectedEntity ? selectedEntity.uuid() : 0;
        m_pendingDeleteUuids.clear();
        ImGui::CloseCurrentPopup();
    } else if (cancel) {
        m_pendingDeleteUuids.clear();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void SceneHierarchyPanel::onImGuiRender(
    vshade::scene::Entity& selectedEntity
) {
    ImGui::Begin("Hierarchy", nullptr, panelFlags);

    const std::uint64_t externalSelection = selectedEntity
        ? selectedEntity.uuid()
        : 0;
    if (externalSelection != m_primarySelectionUuid) {
        m_selectedUuids.clear();
        if (externalSelection != 0) {
            m_selectedUuids.insert(externalSelection);
        }
        m_primarySelectionUuid = externalSelection;
    }

    ImGui::BeginDisabled(m_readOnly || !m_scene);
    if (ImGui::Button("+ Create")) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        selectedEntity = m_scene->create("Entity");
        appendSiblingOrder(selectedEntity);
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
        m_selectedUuids = {selectedEntity.uuid()};
        m_primarySelectionUuid = selectedEntity.uuid();
        beginRename(selectedEntity);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::InputTextWithHint(
        "##HierarchySearch",
        "Search entities...",
        m_searchBuffer.data(),
        m_searchBuffer.size()
    );
    ImGui::Separator();

    if (m_scene) {
        EntityCommand pendingCommand;
        if (!m_readOnly
            && selectedEntity
            && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
            && ImGui::IsKeyPressed(ImGuiKey_F2)) {
            beginRename(selectedEntity);
        }
        if (!m_readOnly
            && selectedEntity
            && m_renamingUuid == 0
            && !ImGui::GetIO().WantTextInput
            && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                pendingCommand = {EntityAction::Delete, selectedEntity};
            } else if (ImGui::GetIO().KeyCtrl
                && ImGui::IsKeyPressed(ImGuiKey_D)) {
                pendingCommand = {EntityAction::Duplicate, selectedEntity};
            }
        }

        for (const auto entity : orderedRoots()) {
            if (!matchesFilter(entity)) {
                continue;
            }
            const EntityCommand command = drawEntity(entity, selectedEntity);
            if (command.action != EntityAction::None) {
                pendingCommand = command;
            }
        }

        const ImVec2 leftover = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton(
            "##HierarchyEmpty",
            {
                std::max(leftover.x, 1.0F),
                std::max(leftover.y, 24.0F),
            }
        );
        if (ImGui::IsItemClicked() && !ImGui::GetIO().KeyCtrl) {
            selectedEntity = {};
            m_selectedUuids.clear();
            m_primarySelectionUuid = 0;
        }
        if (!m_readOnly && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload(entityDragDropType)) {
                const auto uuid =
                    *static_cast<const std::uint64_t*>(payload->Data);
                const auto child = m_scene->findEntity(uuid);
                const auto dragged = commandEntities(child);
                if (!dragged.empty()) {
                    if (m_editHooks.begin) {
                        m_editHooks.begin();
                    }
                    for (const auto target : dragged) {
                        if (m_scene->valid(target) && m_scene->parent(target)) {
                            m_scene->clearParent(target);
                            appendSiblingOrder(target);
                        }
                    }
                    if (m_editHooks.commit) {
                        m_editHooks.commit();
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (!m_readOnly) {
            const auto targets = commandEntities(pendingCommand.entity);
            switch (pendingCommand.action) {
                case EntityAction::CreateChild: {
                    if (m_scene->valid(pendingCommand.entity)) {
                        if (m_editHooks.begin) {
                            m_editHooks.begin();
                        }
                        selectedEntity = m_scene->create("Entity");
                        m_scene->setParent(selectedEntity, pendingCommand.entity);
                        appendSiblingOrder(selectedEntity);
                        if (m_editHooks.commit) {
                            m_editHooks.commit();
                        }
                        m_selectedUuids = {selectedEntity.uuid()};
                        m_primarySelectionUuid = selectedEntity.uuid();
                        beginRename(selectedEntity);
                    }
                    break;
                }
                case EntityAction::Duplicate: {
                    if (m_editHooks.begin) {
                        m_editHooks.begin();
                    }
                    m_selectedUuids.clear();
                    for (const auto target : targets) {
                        if (m_scene->valid(target)) {
                            const auto duplicate = m_scene->duplicateEntity(target);
                            appendSiblingOrder(duplicate);
                            m_selectedUuids.insert(duplicate.uuid());
                            selectedEntity = duplicate;
                        }
                    }
                    if (m_editHooks.commit) {
                        m_editHooks.commit();
                    }
                    m_primarySelectionUuid = selectedEntity
                        ? selectedEntity.uuid()
                        : 0;
                    break;
                }
                case EntityAction::Delete:
                    m_pendingDeleteUuids.clear();
                    for (const auto target : targets) {
                        m_pendingDeleteUuids.push_back(target.uuid());
                    }
                    m_openDeleteConfirmation = !m_pendingDeleteUuids.empty();
                    break;
                case EntityAction::Unparent: {
                    if (m_editHooks.begin) {
                        m_editHooks.begin();
                    }
                    for (const auto target : targets) {
                        if (m_scene->valid(target) && m_scene->parent(target)) {
                            m_scene->clearParent(target);
                            appendSiblingOrder(target);
                        }
                    }
                    if (m_editHooks.commit) {
                        m_editHooks.commit();
                    }
                    break;
                }
                case EntityAction::MoveUp:
                case EntityAction::MoveDown:
                    if (m_scene->valid(pendingCommand.entity)) {
                        if (m_editHooks.begin) {
                            m_editHooks.begin();
                        }
                        moveSibling(
                            pendingCommand.entity,
                            pendingCommand.action == EntityAction::MoveUp
                                ? -1
                                : 1
                        );
                        if (m_editHooks.commit) {
                            m_editHooks.commit();
                        }
                    }
                    break;
                case EntityAction::None:
                    break;
            }
        }
    }

    drawDeleteConfirmation(selectedEntity);
    ImGui::End();
}

SceneHierarchyPanel::EntityCommand SceneHierarchyPanel::drawEntity(
    vshade::scene::Entity entity,
    vshade::scene::Entity& selectedEntity
) {
    const auto children = orderedChildren(entity);
    const bool hasChildren = !children.empty();
    const bool selected = m_selectedUuids.contains(entity.uuid());
    const bool renaming = m_renamingUuid == entity.uuid();
    const bool isPrefab = prefabMember(*m_scene, entity);
    const bool isPrefabRoot =
        entity.has<vshade::scene::PrefabInstanceComponent>();
    const auto* state = entity.tryGet<vshade::scene::HierarchyStateComponent>();
    const vshade::scene::HierarchyStateComponent hierarchyState =
        state ? *state : vshade::scene::HierarchyStateComponent{};
    const bool visible = hierarchyState.visible;
    const bool locked = hierarchyState.locked;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_FramePadding
        | ImGuiTreeNodeFlags_DefaultOpen;
    if (selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!hasChildren) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (m_searchBuffer[0] != '\0') {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (isPrefab) {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ui::color(ui::ColorRole::Accent)
        );
    } else if (!visible) {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ui::color(ui::ColorRole::Muted)
        );
    }
    const auto* nodeId = reinterpret_cast<void*>(entity.uuid());
    const std::string label = renaming
        ? std::string(40, ' ')
        : "    " + std::string(entity.name());
    const bool opened = ImGui::TreeNodeEx(nodeId, flags, "%s", label.c_str());
    if (isPrefab || !visible) {
        ImGui::PopStyleColor();
    }

    const ImVec2 rowMinimum = ImGui::GetItemRectMin();
    const ImVec2 rowMaximum = ImGui::GetItemRectMax();
    const ImVec2 nextCursor = ImGui::GetCursorScreenPos();
    const float rowHeight = rowMaximum.y - rowMinimum.y;
    const float rightControlsWidth = ui::scaled(42.0F);
    const float labelStart = rowMinimum.x + ImGui::GetTreeNodeToLabelSpacing();

    if (isPrefab) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImU32 accent = ImGui::GetColorU32(ui::color(ui::ColorRole::Accent));
        if (isPrefabRoot) {
            drawList->AddRect(
                rowMinimum,
                rowMaximum,
                accent,
                ui::scaled(3.0F),
                0,
                ui::scaled(1.0F)
            );
        } else {
            drawList->AddLine(
                {rowMinimum.x + ui::scaled(1.0F), rowMinimum.y},
                {rowMinimum.x + ui::scaled(1.0F), rowMaximum.y},
                accent,
                ui::scaled(2.0F)
            );
        }
    }

    if (const auto icon = entityIcon(entity)) {
        const float iconSize = ui::scaled(14.0F);
        const float iconY = rowMinimum.y + (rowHeight - iconSize) * 0.5F;
        ImGui::GetWindowDrawList()->AddImage(
            ImTextureRef{static_cast<ImTextureID>(icon->rendererId())},
            {labelStart, iconY},
            {labelStart + iconSize, iconY + iconSize}
        );
    }

    const bool rowClicked = ImGui::IsItemClicked()
        && !ImGui::IsItemToggledOpen()
        && ImGui::GetMousePos().x < rowMaximum.x - rightControlsWidth;
    if (rowClicked && !locked) {
        if (ImGui::GetIO().KeyCtrl) {
            if (selected) {
                m_selectedUuids.erase(entity.uuid());
            } else {
                m_selectedUuids.insert(entity.uuid());
            }
            if (m_selectedUuids.empty()) {
                selectedEntity = {};
                m_primarySelectionUuid = 0;
            } else {
                selectedEntity = entity;
                m_primarySelectionUuid = entity.uuid();
            }
        } else {
            m_selectedUuids = {entity.uuid()};
            selectedEntity = entity;
            m_primarySelectionUuid = entity.uuid();
        }
    }
    if (!m_readOnly && !locked
        && ImGui::IsItemHovered()
        && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        beginRename(entity);
    }

    if (renaming) {
        ImGui::SetCursorScreenPos({
            labelStart + ui::scaled(18.0F),
            rowMinimum.y,
        });
        ImGui::SetNextItemWidth(std::max(
            rowMaximum.x - rightControlsWidth - labelStart - ui::scaled(20.0F),
            ui::scaled(60.0F)
        ));
        if (m_focusRename) {
            ImGui::SetKeyboardFocusHere();
            m_focusRename = false;
        }
        const bool submitted = ImGui::InputText(
            "##RenameEntity",
            m_renameBuffer.data(),
            m_renameBuffer.size(),
            ImGuiInputTextFlags_EnterReturnsTrue
                | ImGuiInputTextFlags_AutoSelectAll
        );
        const bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape);
        const bool finish = submitted || ImGui::IsItemDeactivatedAfterEdit();
        if (cancel) {
            m_renamingUuid = 0;
            m_renameValidationError = false;
        } else if (finish) {
            const std::string name = m_renameBuffer.data();
            if (name.empty()) {
                m_renameValidationError = true;
                m_focusRename = true;
            } else {
                if (name != entity.name()) {
                    if (m_editHooks.begin) {
                        m_editHooks.begin();
                    }
                    entity.setName(name);
                    if (m_editHooks.commit) {
                        m_editHooks.commit();
                    }
                }
                m_renamingUuid = 0;
                m_renameValidationError = false;
            }
        }
        if (m_renameValidationError && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Entity name cannot be empty.");
        }
    }

    ImGui::PushID(static_cast<int>(entity.id()));
    const auto drawStateButton = [&](
        const char* id,
        const bool active,
        const bool lockButton
    ) {
        const ImVec2 size{ui::scaled(18.0F), rowHeight};
        const bool pressed = ImGui::InvisibleButton(id, size);
        const ImVec2 minimum = ImGui::GetItemRectMin();
        const ImVec2 maximum = ImGui::GetItemRectMax();
        const ImVec2 center{
            (minimum.x + maximum.x) * 0.5F,
            (minimum.y + maximum.y) * 0.5F,
        };
        const ImU32 color = ImGui::GetColorU32(
            active ? ImGuiCol_Text : ImGuiCol_TextDisabled
        );
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (lockButton) {
            drawList->AddRect(
                {center.x - 4.0F, center.y - 1.0F},
                {center.x + 4.0F, center.y + 6.0F},
                color,
                1.0F,
                0,
                1.4F
            );
            drawList->PathArcTo(
                {center.x, center.y - 1.0F},
                3.5F,
                3.14159265F,
                6.2831853F,
                10
            );
            drawList->PathStroke(color, 0, 1.4F);
        } else {
            drawList->AddCircle(center, 5.0F, color, 16, 1.3F);
            drawList->AddCircleFilled(center, 1.8F, color, 12);
            if (!active) {
                drawList->AddLine(
                    {center.x - 5.0F, center.y + 5.0F},
                    {center.x + 5.0F, center.y - 5.0F},
                    color,
                    1.5F
                );
            }
        }
        return pressed;
    };

    ImGui::SetCursorScreenPos({rowMaximum.x - rightControlsWidth, rowMinimum.y});
    if (!m_readOnly && drawStateButton("##Visible", visible, false)) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        auto next = hierarchyState;
        next.visible = !visible;
        entity.set<vshade::scene::HierarchyStateComponent>(next);
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(visible ? "Hide entity" : "Show entity");
    }
    ImGui::SameLine(0.0F, ui::scaled(2.0F));
    if (!m_readOnly && drawStateButton("##Locked", locked, true)) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        auto next = hierarchyState;
        next.locked = !locked;
        entity.set<vshade::scene::HierarchyStateComponent>(next);
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(locked ? "Unlock entity" : "Lock entity");
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(nextCursor);

    if (!m_readOnly && !locked && ImGui::BeginDragDropSource()) {
        const std::uint64_t uuid = entity.uuid();
        ImGui::SetDragDropPayload(entityDragDropType, &uuid, sizeof(uuid));
        const std::string_view name = entity.name();
        ImGui::TextUnformatted(name.data(), name.data() + name.size());
        ImGui::EndDragDropSource();
    }
    if (!m_readOnly && !locked && ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(entityDragDropType)) {
            const auto uuid = *static_cast<const std::uint64_t*>(payload->Data);
            const auto child = m_scene->findEntity(uuid);
            const auto dragged = commandEntities(child);
            if (child && child != entity && !dragged.empty()) {
                try {
                    if (m_editHooks.begin) {
                        m_editHooks.begin();
                    }
                    for (const auto target : dragged) {
                        if (target != entity && m_scene->valid(target)) {
                            m_scene->setParent(target, entity);
                            appendSiblingOrder(target);
                        }
                    }
                    if (m_editHooks.commit) {
                        m_editHooks.commit();
                    }
                } catch (const std::exception& error) {
                    if (m_editHooks.cancel) {
                        m_editHooks.cancel();
                    }
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
        if (ImGui::MenuItem("Rename", "F2", false, !locked)) {
            beginRename(entity);
        }
        if (ImGui::MenuItem("Create Child Entity", nullptr, false, !locked)) {
            command = {EntityAction::CreateChild, entity};
        }
        if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, !locked)) {
            command = {EntityAction::Duplicate, entity};
        }
        if (ImGui::MenuItem("Move Up", nullptr, false, !locked)) {
            command = {EntityAction::MoveUp, entity};
        }
        if (ImGui::MenuItem("Move Down", nullptr, false, !locked)) {
            command = {EntityAction::MoveDown, entity};
        }
        if (ImGui::MenuItem("Save as Prefab...") && m_savePrefab) {
            m_savePrefab(entity);
        }
        if (m_scene->parent(entity)
            && ImGui::MenuItem("Unparent", nullptr, false, !locked)) {
            command = {EntityAction::Unparent, entity};
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Delete", false, !locked)) {
            command = {EntityAction::Delete, entity};
        }
        ImGui::EndPopup();
    }

    if (opened && hasChildren) {
        for (const auto child : children) {
            if (!matchesFilter(child)) {
                continue;
            }
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
