#pragma once

#include "SceneEditHooks.hpp"

#include <scene/Entity.hpp>

#include <filesystem>
#include <functional>

namespace vshade::asset {
class AssetManager;
}

namespace vshade::script {
class NativeScriptRegistry;
}

namespace editor {

class InspectorPanel final {
public:
    void setAssetManager(vshade::asset::AssetManager& assets) noexcept;
    void setScriptRegistry(vshade::script::NativeScriptRegistry& scripts) noexcept;
    void setEditHooks(SceneEditHooks hooks);
    void setRevealAssetHandler(std::function<void(std::filesystem::path)> handler);
    void onImGuiRender(vshade::scene::Entity selectedEntity);

private:
    void drawTag(vshade::scene::Entity entity);
    void drawPrefab(vshade::scene::Entity entity);
    void drawTransform(vshade::scene::Entity entity);
    void drawCamera(vshade::scene::Entity entity);
    void drawSpriteRenderer(vshade::scene::Entity entity);
    void drawModelRenderer(vshade::scene::Entity entity);
    void drawAudioSource(vshade::scene::Entity entity);
    void drawAudioListener(vshade::scene::Entity entity);
    void drawLight(vshade::scene::Entity entity);
    void drawRigidBody2D(vshade::scene::Entity entity);
    void drawCollider2D(vshade::scene::Entity entity);
    void drawRigidBody3D(vshade::scene::Entity entity);
    void drawCollider3D(vshade::scene::Entity entity);
    void drawScripts(vshade::scene::Entity entity);
    void drawAddComponentMenu(vshade::scene::Entity entity);

    vshade::asset::AssetManager* m_assets = nullptr;
    vshade::script::NativeScriptRegistry* m_scripts = nullptr;
    SceneEditHooks m_editHooks;
    std::function<void(std::filesystem::path)> m_revealAsset;
};

} // namespace editor
