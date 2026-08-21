#pragma once

#include "SceneEditHooks.hpp"

#include <scene/Entity.hpp>

#include <functional>
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace vshade::scene {
class Scene;
}

namespace editor {

class SceneHierarchyPanel final {
public:
    void setScene(std::shared_ptr<vshade::scene::Scene> scene);
    void setReadOnly(bool readOnly) noexcept;
    void setSavePrefabHandler(std::function<void(vshade::scene::Entity)> handler);
    void setEditHooks(SceneEditHooks hooks);
    void onImGuiRender(vshade::scene::Entity& selectedEntity);

private:
    enum class EntityAction {
        None,
        Duplicate,
        Delete,
        Unparent,
        CreateChild,
        MoveUp,
        MoveDown,
    };

    struct EntityCommand {
        EntityAction action = EntityAction::None;
        vshade::scene::Entity entity;
    };

    [[nodiscard]] EntityCommand drawEntity(
        vshade::scene::Entity entity,
        vshade::scene::Entity& selectedEntity
    );
    [[nodiscard]] std::vector<vshade::scene::Entity> orderedChildren(
        vshade::scene::Entity parent
    ) const;
    [[nodiscard]] std::vector<vshade::scene::Entity> orderedRoots() const;
    [[nodiscard]] bool matchesFilter(vshade::scene::Entity entity) const;
    void beginRename(vshade::scene::Entity entity);
    void drawDeleteConfirmation(vshade::scene::Entity& selectedEntity);
    void moveSibling(vshade::scene::Entity entity, int direction);
    void appendSiblingOrder(vshade::scene::Entity entity);
    [[nodiscard]] std::vector<vshade::scene::Entity> commandEntities(
        vshade::scene::Entity contextEntity
    ) const;
    [[nodiscard]] std::vector<vshade::scene::Entity> subtree(
        vshade::scene::Entity root
    ) const;

    std::shared_ptr<vshade::scene::Scene> m_scene;
    std::function<void(vshade::scene::Entity)> m_savePrefab;
    SceneEditHooks m_editHooks;
    std::array<char, 128> m_searchBuffer{};
    std::array<char, 256> m_renameBuffer{};
    std::unordered_set<std::uint64_t> m_selectedUuids;
    std::vector<std::uint64_t> m_pendingDeleteUuids;
    std::uint64_t m_primarySelectionUuid = 0;
    std::uint64_t m_renamingUuid = 0;
    bool m_focusRename = false;
    bool m_renameValidationError = false;
    bool m_openDeleteConfirmation = false;
    bool m_readOnly = false;
};

} // namespace editor
