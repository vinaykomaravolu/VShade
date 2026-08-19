#pragma once

namespace editor {

class SceneHierarchyPanel final {
public:
    void onImGuiRender();
    [[nodiscard]] int selectedEntity() const noexcept;

private:
    int m_selectedEntity = 0;
};

} // namespace editor
