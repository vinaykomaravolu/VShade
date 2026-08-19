#pragma once

#include <scene/Entity.hpp>

namespace editor {

class InspectorPanel final {
public:
    void onImGuiRender(vshade::scene::Entity selectedEntity);
};

} // namespace editor
