#pragma once

namespace editor {

class InspectorPanel final {
public:
    void onImGuiRender(int selectedEntity);

private:
    float m_position[3]{0.0F, 0.0F, 0.0F};
    float m_rotation[3]{0.0F, 0.0F, 0.0F};
    float m_scale[3]{1.0F, 1.0F, 1.0F};
};

} // namespace editor
