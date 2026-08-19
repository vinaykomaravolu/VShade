#pragma once

#include <scene/Entity.hpp>

#include <memory>

namespace vshade::scene {
class Scene;
}

namespace editor {

class SceneHierarchyPanel final {
public:
    void setScene(std::shared_ptr<vshade::scene::Scene> scene);
    void onImGuiRender();
    [[nodiscard]] vshade::scene::Entity selectedEntity() const noexcept;

private:
    std::shared_ptr<vshade::scene::Scene> m_scene;
    vshade::scene::Entity m_selectedEntity;
};

} // namespace editor
