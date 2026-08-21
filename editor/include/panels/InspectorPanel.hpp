#pragma once

#include "SceneEditHooks.hpp"

#include <scene/Entity.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>

namespace vshade::asset {
class AssetManager;
}

namespace vshade::script {
class NativeScriptRegistry;
}

namespace editor {

class AssetSelector;

class InspectorPanel final {
public:
    void setAssetManager(vshade::asset::AssetManager& assets) noexcept;
    void setAssetSelector(AssetSelector& selector) noexcept;
    void setScriptRegistry(vshade::script::NativeScriptRegistry& scripts) noexcept;
    void setEditHooks(SceneEditHooks hooks);
    void setReadOnly(bool readOnly) noexcept;
    void setRevealAssetHandler(std::function<void(std::filesystem::path)> handler);
    void setSelectedAsset(std::filesystem::path path);
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
    void drawTypedColliders3D(vshade::scene::Entity entity);
    void drawScripts(vshade::scene::Entity entity);
    void drawAddComponentMenu(vshade::scene::Entity entity);
    void drawAssetInspector();
    void drawTextureAsset(const std::filesystem::path& path);
    void drawAudioAsset(const std::filesystem::path& path);
    void drawModelAsset(const std::filesystem::path& path);

    vshade::asset::AssetManager* m_assets = nullptr;
    AssetSelector* m_assetSelector = nullptr;
    vshade::script::NativeScriptRegistry* m_scripts = nullptr;
    SceneEditHooks m_editHooks;
    std::function<void(std::filesystem::path)> m_revealAsset;
    std::filesystem::path m_selectedAsset;
    std::array<char, 256> m_nameBuffer{};
    std::array<char, 128> m_componentSearch{};
    std::uint64_t m_nameEntityUuid = 0;
    bool m_nameValidationError = false;
    bool m_interactionRecording = false;
    bool m_readOnly = false;
};

} // namespace editor
