#pragma once

#include <scene/Entity.hpp>

namespace editor {

class InspectorPanel final {
public:
    void onImGuiRender(vshade::scene::Entity selectedEntity);

private:
    void drawTag(vshade::scene::Entity entity);
    void drawTransform(vshade::scene::Entity entity);
};

} // namespace editor
