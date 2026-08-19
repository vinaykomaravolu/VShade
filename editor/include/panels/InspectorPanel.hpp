#pragma once

#include "SceneEditHooks.hpp"

#include <scene/Entity.hpp>

namespace vshade::asset {
class AssetManager;
}

namespace editor {

class InspectorPanel final {
public:
    void setAssetManager(vshade::asset::AssetManager& assets) noexcept;
    void setEditHooks(SceneEditHooks hooks);
    void onImGuiRender(vshade::scene::Entity selectedEntity);

private:
    void drawTag(vshade::scene::Entity entity);
    void drawTransform(vshade::scene::Entity entity);
    void drawCamera(vshade::scene::Entity entity);
    void drawSpriteRenderer(vshade::scene::Entity entity);
    void drawModelRenderer(vshade::scene::Entity entity);
    void drawAudioSource(vshade::scene::Entity entity);
    void drawLight(vshade::scene::Entity entity);
    void drawAddComponentMenu(vshade::scene::Entity entity);

    vshade::asset::AssetManager* m_assets = nullptr;
    SceneEditHooks m_editHooks;
};

} // namespace editor
