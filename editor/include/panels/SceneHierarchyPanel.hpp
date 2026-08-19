#pragma once

#include <scene/Entity.hpp>

#include <functional>
#include <memory>

namespace vshade::scene {
class Scene;
}

namespace editor {

class SceneHierarchyPanel final {
public:
    void setScene(std::shared_ptr<vshade::scene::Scene> scene);
    void setReadOnly(bool readOnly) noexcept;
    void setSavePrefabHandler(std::function<void(vshade::scene::Entity)> handler);
    void onImGuiRender(vshade::scene::Entity& selectedEntity);

private:
    enum class EntityAction {
        None,
        Duplicate,
        Delete,
    };

    [[nodiscard]] EntityAction drawEntity(
        vshade::scene::Entity entity,
        vshade::scene::Entity& selectedEntity
    );

    std::shared_ptr<vshade::scene::Scene> m_scene;
    std::function<void(vshade::scene::Entity)> m_savePrefab;
    bool m_readOnly = false;
};

} // namespace editor
