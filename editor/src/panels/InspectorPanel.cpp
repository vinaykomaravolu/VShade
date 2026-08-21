#include "panels/InspectorPanel.hpp"
#include "ImGui/ImGuiTheme.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <audio/AudioClip.hpp>
#include <math/Quaternion.hpp>
#include <physics/PhysicsTypes.hpp>
#include <physics/physics2d/PhysicsShape2D.hpp>
#include <physics/physics3d/PhysicsShape3D.hpp>
#include <renderer/Lighting.hpp>
#include <renderer/Model.hpp>
#include <scene/Prefab.hpp>
#include <scene/Scene.hpp>
#include <scene/components/AudioComponents.hpp>
#include <scene/components/CoreComponents.hpp>
#include <scene/components/LightComponent.hpp>
#include <scene/components/PhysicsComponents2D.hpp>
#include <scene/components/PhysicsComponents3D.hpp>
#include <scene/components/PrefabInstanceComponent.hpp>
#include <scene/components/RenderComponents.hpp>
#include <scene/components/ScriptComponent.hpp>
#include <script/NativeScriptRegistry.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <glm/trigonometric.hpp>
#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

template<typename Action>
void commitMutation(const SceneEditHooks& hooks, Action&& action) {
    if (hooks.begin) {
        hooks.begin();
    }
    std::forward<Action>(action)();
    if (hooks.commit) {
        hooks.commit();
    }
}

template<typename Component, typename UiFunction, typename DropFunction>
void drawComponent(
    const char* name,
    vshade::scene::Entity entity,
    const SceneEditHooks& hooks,
    UiFunction&& uiFunction,
    DropFunction&& dropFunction
) {
    if (!entity.has<Component>()) {
        return;
    }

    ImGui::PushID(name);
    const bool open = ImGui::CollapsingHeader(
        name,
        ImGuiTreeNodeFlags_DefaultOpen
    );
    const ImVec2 headerMinimum = ImGui::GetItemRectMin();
    const ImVec2 headerMaximum = ImGui::GetItemRectMax();
    const ImVec2 nextCursor = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos({
        headerMaximum.x - ui::scaled(28.0F),
        headerMinimum.y + ui::scaled(2.0F),
    });
    if (ImGui::SmallButton("...")) {
        ImGui::OpenPopup("ComponentActions");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Component actions");
    }
    ImGui::SetCursorScreenPos(nextCursor);

    static std::optional<Component> copiedValues;
    bool reset = false;
    bool remove = false;
    bool paste = false;
    if (ImGui::BeginPopup("ComponentActions")) {
        if (ImGui::MenuItem("Copy Values")) {
            copiedValues = entity.get<Component>();
        }
        ImGui::BeginDisabled(!copiedValues.has_value());
        paste = ImGui::MenuItem("Paste Values");
        ImGui::EndDisabled();
        ImGui::Separator();
        reset = ImGui::MenuItem("Reset");
        remove = ImGui::MenuItem("Remove Component");
        ImGui::EndPopup();
    }
    if (paste) {
        commitMutation(hooks, [&] { entity.set<Component>(*copiedValues); });
        ImGui::PopID();
        return;
    }
    if (reset) {
        commitMutation(hooks, [&] { entity.set<Component>(Component{}); });
        ImGui::PopID();
        return;
    }
    if (remove) {
        commitMutation(hooks, [&] { entity.remove<Component>(); });
        ImGui::PopID();
        return;
    }

    auto& component = entity.get<Component>();
    std::forward<DropFunction>(dropFunction)(component);
    if (open) {
        std::forward<UiFunction>(uiFunction)(component);
    }
    ImGui::PopID();
}

template<typename Component, typename UiFunction>
void drawComponent(
    const char* name,
    vshade::scene::Entity entity,
    const SceneEditHooks& hooks,
    UiFunction&& uiFunction
) {
    drawComponent<Component>(
        name,
        entity,
        hooks,
        std::forward<UiFunction>(uiFunction),
        [](Component&) {}
    );
}

void trackTransformItem(const SceneEditHooks& hooks) {
    if (ImGui::IsItemActivated() && hooks.begin) {
        hooks.begin();
    }
    if (ImGui::IsItemDeactivated() && hooks.commit) {
        hooks.commit();
    }
}

void activateExclusiveListener(vshade::scene::Entity entity) {
    auto& selected = entity.get<vshade::scene::AudioListenerComponent>();
    selected.active = true;
    for (auto [handle, listener] :
         entity.scene().view<vshade::scene::AudioListenerComponent>().each()) {
        if (handle != entity.handle()) {
            listener.active = false;
        }
    }
}

void drawBodyType(vshade::physics::BodyType& type) {
    int value = static_cast<int>(type);
    constexpr const char* names[] = {"Static", "Dynamic", "Kinematic"};
    if (ImGui::Combo("Body Type", &value, names, IM_ARRAYSIZE(names))) {
        type = static_cast<vshade::physics::BodyType>(value);
    }
}

} // namespace

void InspectorPanel::setAssetManager(
    vshade::asset::AssetManager& assets
) noexcept {
    m_assets = &assets;
}

void InspectorPanel::setScriptRegistry(
    vshade::script::NativeScriptRegistry& scripts
) noexcept {
    m_scripts = &scripts;
}

void InspectorPanel::setEditHooks(SceneEditHooks hooks) {
    m_editHooks = std::move(hooks);
}

template<typename Resource>
void drawBrokenAssetWarning(
    const char* label,
    const vshade::asset::AssetReference<Resource>& reference,
    const vshade::asset::AssetManager* assets
) {
    if (!reference.valid() || assets == nullptr) {
        return;
    }
    std::filesystem::path path = reference.sourcePath();
    if (path.is_relative()) {
        path = assets->rootDirectory() / path;
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        ImGui::TextColored(
            ui::color(ui::ColorRole::Warning),
            "Missing %s asset: %s",
            label,
            reference.sourcePath().generic_string().c_str()
        );
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "The reference is preserved, but its source file cannot be found."
            );
        }
    }
}

[[nodiscard]] bool hasTypedCollider3D(const vshade::scene::Entity entity) {
    return entity.has<vshade::scene::BoxCollider3DComponent>()
        || entity.has<vshade::scene::SphereCollider3DComponent>()
        || entity.has<vshade::scene::CapsuleCollider3DComponent>()
        || entity.has<vshade::scene::CylinderCollider3DComponent>()
        || entity.has<vshade::scene::MeshCollider3DComponent>()
        || entity.has<vshade::scene::ConvexCollider3DComponent>();
}

[[nodiscard]] bool hasAnyCollider3D(const vshade::scene::Entity entity) {
    return entity.has<vshade::scene::Collider3DComponent>()
        || hasTypedCollider3D(entity);
}

[[nodiscard]] std::shared_ptr<vshade::renderer::Model> entityModel(
    const vshade::scene::Entity entity,
    vshade::asset::AssetManager* assets
) {
    const auto* modelRenderer =
        entity.tryGet<vshade::scene::ModelRendererComponent>();
    if (assets == nullptr || modelRenderer == nullptr
        || !modelRenderer->model.valid()) {
        return {};
    }
    try {
        return assets->loadResource(modelRenderer->model).shared();
    } catch (const std::exception& error) {
        ENGINE_WARN(
            "Could not fit collider for '{}': {}",
            entity.name(),
            error.what()
        );
        return {};
    }
}

template<typename Component>
bool fitColliderToModel(
    const vshade::scene::Entity entity,
    vshade::asset::AssetManager* assets,
    Component& component
) {
    const auto* modelRenderer =
        entity.tryGet<vshade::scene::ModelRendererComponent>();
    const auto model = entityModel(entity, assets);
    if (!modelRenderer || !model || !model->localBounds()) {
        return false;
    }

    if constexpr (
        std::is_same_v<Component, vshade::scene::MeshCollider3DComponent>
        || std::is_same_v<Component, vshade::scene::ConvexCollider3DComponent>
    ) {
        component.model = modelRenderer->model;
        return true;
    } else {
        constexpr float minimumSize = 0.001F;
        const auto& bounds = *model->localBounds();
        const vshade::math::Vec3 center =
            (bounds.minimum + bounds.maximum) * 0.5F;
        const vshade::math::Vec3 extents = glm::max(
            (bounds.maximum - bounds.minimum) * 0.5F,
            vshade::math::Vec3{minimumSize}
        );
        component.offset = center;
        if constexpr (
            std::is_same_v<Component, vshade::scene::BoxCollider3DComponent>
        ) {
            component.halfExtents = extents;
        } else if constexpr (
            std::is_same_v<Component, vshade::scene::SphereCollider3DComponent>
        ) {
            component.radius = std::max({extents.x, extents.y, extents.z});
        } else if constexpr (
            std::is_same_v<Component, vshade::scene::CapsuleCollider3DComponent>
        ) {
            component.radius = std::max(extents.x, extents.z);
            component.halfHeight = std::max(
                extents.y - component.radius,
                minimumSize
            );
        } else if constexpr (
            std::is_same_v<Component, vshade::scene::CylinderCollider3DComponent>
        ) {
            component.radius = std::max(extents.x, extents.z);
            component.halfHeight = extents.y;
        }
        return true;
    }
}

template<typename Component>
void drawColliderProperties(Component& component) {
    ImGui::DragFloat3("Offset", &component.offset.x, 0.05F);
    ImGui::DragFloat("Friction", &component.material.friction, 0.01F);
    ImGui::DragFloat("Restitution", &component.material.restitution, 0.01F);
    component.material.friction = std::max(component.material.friction, 0.0F);
    component.material.restitution =
        std::max(component.material.restitution, 0.0F);
    ImGui::Checkbox("Sensor", &component.sensor);
}

void InspectorPanel::setAssetSelector(AssetSelector& selector) noexcept {
    m_assetSelector = &selector;
}

void InspectorPanel::setReadOnly(const bool readOnly) noexcept {
    m_readOnly = readOnly;
    if (readOnly) {
        m_interactionRecording = false;
    }
}

void InspectorPanel::setRevealAssetHandler(
    std::function<void(std::filesystem::path)> handler
) {
    m_revealAsset = std::move(handler);
}

void InspectorPanel::onImGuiRender(
    vshade::scene::Entity selectedEntity
) {
    ImGui::Begin("Inspector", nullptr, panelFlags);

    const ImVec4 stateColor = m_readOnly
        ? ui::color(ui::ColorRole::Warning)
        : ui::color(ui::ColorRole::Success);
    ImGui::TextColored(
        stateColor,
        m_readOnly ? "RUNTIME  |  READ ONLY" : "EDIT MODE"
    );
    if (m_readOnly) {
        ImGui::SameLine();
        ImGui::TextDisabled("Stop Play mode to edit scene values.");
    }
    ImGui::Separator();

    const bool popupOpen = ImGui::IsPopupOpen(
        nullptr,
        ImGuiPopupFlags_AnyPopupId
    );
    const bool inspectorHovered = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_RootAndChildWindows
    );
    const bool inspectorFocused = ImGui::IsWindowFocused(
        ImGuiFocusedFlags_RootAndChildWindows
    );
    const bool editKeyPressed =
        ImGui::IsKeyPressed(ImGuiKey_Enter)
        || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)
        || ImGui::IsKeyPressed(ImGuiKey_Space)
        || ImGui::IsKeyPressed(ImGuiKey_Backspace)
        || ImGui::IsKeyPressed(ImGuiKey_Delete)
        || ImGui::IsKeyPressed(ImGuiKey_LeftArrow)
        || ImGui::IsKeyPressed(ImGuiKey_RightArrow)
        || ImGui::IsKeyPressed(ImGuiKey_UpArrow)
        || ImGui::IsKeyPressed(ImGuiKey_DownArrow)
        || ImGui::IsKeyPressed(ImGuiKey_Home)
        || ImGui::IsKeyPressed(ImGuiKey_End);
    if (!m_readOnly
        && selectedEntity
        && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        && (inspectorHovered || popupOpen)) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        m_interactionRecording = true;
    }
    if (!m_readOnly
        && selectedEntity
        && (inspectorFocused || popupOpen)
        && editKeyPressed
        && !m_interactionRecording) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        m_interactionRecording = true;
    }
    if (!m_readOnly
        && selectedEntity
        && inspectorHovered
        && ImGui::GetDragDropPayload() != nullptr
        && !m_interactionRecording) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        m_interactionRecording = true;
    }

    ImGui::BeginDisabled(m_readOnly);
    ImGui::PushItemWidth(-ui::scaled(145.0F));

    if (selectedEntity) {
        if (m_nameEntityUuid != selectedEntity.uuid()) {
            m_nameBuffer.fill('\0');
            const std::string_view name = selectedEntity.name();
            const std::size_t count = std::min(
                name.size(),
                m_nameBuffer.size() - 1
            );
            std::memcpy(m_nameBuffer.data(), name.data(), count);
            m_nameEntityUuid = selectedEntity.uuid();
            m_nameValidationError = false;
        }
        drawTag(selectedEntity);
        ImGui::Separator();
        drawPrefab(selectedEntity);
        drawTransform(selectedEntity);
        drawCamera(selectedEntity);
        drawSpriteRenderer(selectedEntity);
        drawModelRenderer(selectedEntity);
        drawAudioSource(selectedEntity);
        drawAudioListener(selectedEntity);
        drawLight(selectedEntity);
        drawRigidBody2D(selectedEntity);
        drawCollider2D(selectedEntity);
        drawRigidBody3D(selectedEntity);
        drawCollider3D(selectedEntity);
        drawTypedColliders3D(selectedEntity);
        drawScripts(selectedEntity);
        ImGui::Separator();
        drawAddComponentMenu(selectedEntity);
    } else {
        m_nameEntityUuid = 0;
        m_nameValidationError = false;
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::PopItemWidth();
    ImGui::EndDisabled();

    if (m_interactionRecording
        && !ImGui::IsMouseDown(ImGuiMouseButton_Left)
        && !ImGui::IsAnyItemActive()
        && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
        m_interactionRecording = false;
    }

    ImGui::End();
}

void InspectorPanel::drawTag(vshade::scene::Entity entity) {
    if (!entity.has<vshade::scene::TagComponent>()) {
        return;
    }

    const bool submitted = ImGui::InputText(
        "Name",
        m_nameBuffer.data(),
        m_nameBuffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue
    );
    if (ImGui::IsItemActivated() && m_editHooks.begin) {
        m_editHooks.begin();
    }
    if (submitted || ImGui::IsItemDeactivatedAfterEdit()) {
        const std::string name = m_nameBuffer.data();
        if (name.empty()) {
            m_nameValidationError = true;
            m_nameBuffer.fill('\0');
            const std::string_view validName = entity.name();
            const std::size_t count = std::min(
                validName.size(),
                m_nameBuffer.size() - 1
            );
            std::memcpy(m_nameBuffer.data(), validName.data(), count);
            if (m_editHooks.cancel) {
                m_editHooks.cancel();
            }
        } else {
            m_nameValidationError = false;
            if (name != entity.name()) {
                entity.setName(name);
            }
            if (m_editHooks.commit) {
                m_editHooks.commit();
            }
        }
    }
    if (m_nameValidationError) {
        ImGui::TextColored(
            ui::color(ui::ColorRole::Error),
            "Name cannot be empty."
        );
    }
}

void InspectorPanel::drawPrefab(vshade::scene::Entity entity) {
    if (!entity.has<vshade::scene::PrefabInstanceComponent>()) {
        return;
    }

    auto& instance = entity.get<vshade::scene::PrefabInstanceComponent>();
    if (!ImGui::CollapsingHeader("Prefab", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const std::string path = instance.prefab.sourcePath().generic_string();
    std::array<char, 512> buffer{};
    const std::size_t characterCount = std::min(path.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), path.data(), characterCount);
    ImGui::InputText(
        "Asset",
        buffer.data(),
        buffer.size(),
        ImGuiInputTextFlags_ReadOnly
    );

    const bool canOpen = static_cast<bool>(m_revealAsset) && !path.empty();
    const bool canApply = m_assets != nullptr && instance.prefab.valid();
    drawBrokenAssetWarning("prefab", instance.prefab, m_assets);

    ImGui::BeginDisabled(!canOpen);
    if (ImGui::Button("Open")) {
        m_revealAsset(instance.prefab.sourcePath());
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!canApply);
    if (ImGui::Button("Apply")) {
        try {
            if (m_editHooks.begin) {
                m_editHooks.begin();
            }
            if (m_assets->isLoaded(instance.prefab.handle())) {
                m_assets->unload(instance.prefab.handle());
            }
            const auto prefab = m_assets->loadResource<vshade::scene::Prefab>(
                instance.prefab
            );
            entity.scene().applyPrefab(entity, *prefab);
            if (m_editHooks.commit) {
                m_editHooks.commit();
            }
        } catch (const std::exception& error) {
            if (m_editHooks.revert) {
                m_editHooks.revert();
            }
            ENGINE_ERROR(
                "Failed to apply prefab '{}': {}",
                path,
                error.what()
            );
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Unpack")) {
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        entity.scene().unpackPrefab(entity);
        if (m_editHooks.commit) {
            m_editHooks.commit();
        }
    }
}

void InspectorPanel::drawTransform(vshade::scene::Entity entity) {
    if (!entity.has<vshade::scene::TransformComponent>()) {
        return;
    }

    auto& transform = entity.transform();
    if (!ImGui::CollapsingHeader(
            "Transform",
            ImGuiTreeNodeFlags_DefaultOpen
        )) {
        return;
    }

    vshade::math::Vec3 position = transform.position();
    if (ImGui::DragFloat3(
            "Position",
            &position.x,
            0.1F,
            0.0F,
            0.0F,
            "%.3f units"
        )) {
        transform.setPosition(position);
    }
    const bool positionHovered = ImGui::IsItemHovered();
    trackTransformItem(m_editHooks);
    if (ImGui::BeginPopupContextItem("PositionActions")) {
        if (ImGui::MenuItem("Reset to (0, 0, 0)")) {
            transform.setPosition({0.0F, 0.0F, 0.0F});
        }
        ImGui::EndPopup();
    }
    if (positionHovered) {
        ImGui::SetTooltip("Local position in scene units. Right-click to reset.");
    }

    vshade::math::Vec3 rotationDegrees =
        glm::degrees(vshade::math::toEuler(transform.rotation()));
    if (ImGui::DragFloat3(
            "Rotation",
            &rotationDegrees.x,
            0.5F,
            0.0F,
            0.0F,
            "%.1f deg"
        )) {
        transform.setRotation(
            vshade::math::fromEuler(glm::radians(rotationDegrees))
        );
    }
    const bool rotationHovered = ImGui::IsItemHovered();
    trackTransformItem(m_editHooks);
    if (ImGui::BeginPopupContextItem("RotationActions")) {
        if (ImGui::MenuItem("Reset to (0, 0, 0)")) {
            transform.setRotation(vshade::math::fromEuler({0.0F, 0.0F, 0.0F}));
        }
        ImGui::EndPopup();
    }
    if (rotationHovered) {
        ImGui::SetTooltip("Local Euler rotation in degrees. Right-click to reset.");
    }

    vshade::math::Vec3 scale = transform.scale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1F, 0.0F, 0.0F, "%.3f")) {
        constexpr float minimumScale = 0.001F;
        scale.x = std::max(scale.x, minimumScale);
        scale.y = std::max(scale.y, minimumScale);
        scale.z = std::max(scale.z, minimumScale);
        transform.setScale(scale);
    }
    const bool scaleHovered = ImGui::IsItemHovered();
    trackTransformItem(m_editHooks);
    if (ImGui::BeginPopupContextItem("ScaleActions")) {
        if (ImGui::MenuItem("Reset to (1, 1, 1)")) {
            transform.setScale({1.0F, 1.0F, 1.0F});
        }
        ImGui::EndPopup();
    }
    if (scaleHovered) {
        ImGui::SetTooltip("Local scale multiplier. Right-click to reset.");
    }
}

void InspectorPanel::drawCamera(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::CameraComponent>(
        "Camera",
        entity,
        m_editHooks,
        [](vshade::scene::CameraComponent& camera) {
            ImGui::Checkbox("Active", &camera.active);

            int projection = camera.projection
                == vshade::scene::CameraProjection::Perspective ? 0 : 1;
            constexpr const char* projectionNames[] = {
                "Perspective",
                "Orthographic",
            };
            if (ImGui::Combo(
                    "Projection",
                    &projection,
                    projectionNames,
                    IM_ARRAYSIZE(projectionNames)
                )) {
                camera.projection = projection == 0
                    ? vshade::scene::CameraProjection::Perspective
                    : vshade::scene::CameraProjection::Orthographic;
            }

            if (camera.projection
                == vshade::scene::CameraProjection::Perspective) {
                float fieldOfViewDegrees =
                    glm::degrees(camera.verticalFieldOfViewRadians);
                if (ImGui::DragFloat(
                        "Vertical FOV",
                        &fieldOfViewDegrees,
                        0.25F,
                        1.0F,
                        179.0F,
                        "%.1f deg"
                    )) {
                    camera.verticalFieldOfViewRadians = glm::radians(
                        std::clamp(fieldOfViewDegrees, 1.0F, 179.0F)
                    );
                }
            } else {
                ImGui::DragFloat(
                    "Orthographic Height",
                    &camera.orthographicHeight,
                    0.1F,
                    0.001F,
                    100000.0F,
                    "%.2f units"
                );
                camera.orthographicHeight =
                    std::max(camera.orthographicHeight, 0.001F);
            }

            ImGui::DragFloat(
                "Near Clip",
                &camera.nearPlane,
                0.01F,
                0.001F,
                100000.0F,
                "%.3f units"
            );
            camera.nearPlane = std::max(camera.nearPlane, 0.001F);
            ImGui::DragFloat(
                "Far Clip",
                &camera.farPlane,
                1.0F,
                0.002F,
                1000000.0F,
                "%.1f units"
            );
            const bool invalidClipRange =
                camera.farPlane <= camera.nearPlane + 0.001F;
            camera.farPlane = std::max(
                camera.farPlane,
                camera.nearPlane + 0.001F
            );
            if (invalidClipRange) {
                ImGui::TextColored(
                    ui::color(ui::ColorRole::Warning),
                    "Far Clip must be greater than Near Clip."
                );
            }
            ImGui::Checkbox("Clear Color", &camera.clearColorEnabled);
            ImGui::Checkbox("Clear Depth", &camera.clearDepthEnabled);
            if (camera.clearColorEnabled) {
                ImGui::ColorEdit4("Background", &camera.clearColor.x);
            }

            int priority = camera.priority;
            if (ImGui::DragInt("Priority", &priority, 1.0F)) {
                camera.priority = priority;
            }
        }
    );
}

void InspectorPanel::drawSpriteRenderer(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::SpriteRendererComponent>(
        "Sprite Renderer",
        entity,
        m_editHooks,
        [this](vshade::scene::SpriteRendererComponent& sprite) {
            if (m_assets
                && m_assetSelector
                && m_assetSelector->draw(
                    "Texture",
                    sprite.texture,
                    *m_assets
                )) {
                sprite.texturePath = sprite.texture
                    ? sprite.texture.sourcePath()
                    : std::filesystem::path{};
            }
            drawBrokenAssetWarning("texture", sprite.texture, m_assets);
            ImGui::ColorEdit4("Color", &sprite.color.x);
            ImGui::DragFloat2("Tiling", &sprite.tiling.x, 0.1F);
            ImGui::DragInt("Sorting Layer", &sprite.sortingLayer, 1.0F);
        },
        [this](vshade::scene::SpriteRendererComponent& sprite) {
            if (m_assets
                && AssetSelector::acceptDroppedAsset(
                    sprite.texture,
                    *m_assets
                )) {
                sprite.texturePath = sprite.texture
                    ? sprite.texture.sourcePath()
                    : std::filesystem::path{};
            }
        }
    );
}

void InspectorPanel::drawModelRenderer(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::ModelRendererComponent>(
        "Model Renderer",
        entity,
        m_editHooks,
        [this](vshade::scene::ModelRendererComponent& modelRenderer) {
            if (m_assets && m_assetSelector) {
                m_assetSelector->draw(
                    "Model",
                    modelRenderer.model,
                    *m_assets
                );
            }
            drawBrokenAssetWarning("model", modelRenderer.model, m_assets);
            ImGui::Checkbox("Visible", &modelRenderer.visible);
        },
        [this](vshade::scene::ModelRendererComponent& modelRenderer) {
            if (m_assets) {
                AssetSelector::acceptDroppedAsset(
                    modelRenderer.model,
                    *m_assets
                );
            }
        }
    );
}

void InspectorPanel::drawAudioSource(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::AudioSourceComponent>(
        "Audio Source",
        entity,
        m_editHooks,
        [this](vshade::scene::AudioSourceComponent& source) {
            if (m_assets
                && m_assetSelector
                && m_assetSelector->draw(
                    "Clip",
                    source.clipAsset,
                    *m_assets
                )) {
                source.clip = source.clipAsset
                    ? source.clipAsset.handle()
                    : vshade::audio::AudioClipHandle{};
            }
            drawBrokenAssetWarning("audio", source.clipAsset, m_assets);
            ImGui::DragFloat("Volume", &source.volume, 0.01F, 0.0F);
            source.volume = std::max(source.volume, 0.0F);
            ImGui::DragFloat("Pitch", &source.pitch, 0.01F, 0.01F);
            source.pitch = std::max(source.pitch, 0.01F);
            ImGui::Checkbox("Looping", &source.looping);
            ImGui::Checkbox("Play On Start", &source.playOnStart);
            ImGui::Checkbox("Spatial", &source.spatial);
        },
        [this](vshade::scene::AudioSourceComponent& source) {
            if (m_assets
                && AssetSelector::acceptDroppedAsset(
                    source.clipAsset,
                    *m_assets
                )) {
                source.clip = source.clipAsset
                    ? source.clipAsset.handle()
                    : vshade::audio::AudioClipHandle{};
            }
        }
    );
}

void InspectorPanel::drawLight(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::LightComponent>(
        "Light",
        entity,
        m_editHooks,
        [](vshade::scene::LightComponent& component) {
            ImGui::Checkbox("Enabled", &component.enabled);

            int type = std::holds_alternative<
                vshade::renderer::DirectionalLight
            >(component.light) ? 0 : 1;
            constexpr const char* typeNames[] = {
                "Directional",
                "Point",
            };
            if (ImGui::Combo(
                    "Type",
                    &type,
                    typeNames,
                    IM_ARRAYSIZE(typeNames)
                )) {
                component.light = type == 0
                    ? vshade::renderer::Light{
                        vshade::renderer::DirectionalLight{}
                    }
                    : vshade::renderer::Light{
                        vshade::renderer::PointLight{}
                    };
            }

            if (auto* directional = std::get_if<
                    vshade::renderer::DirectionalLight
                >(&component.light)) {
                ImGui::DragFloat3(
                    "Direction",
                    &directional->direction.x,
                    0.05F
                );
                ImGui::ColorEdit3("Color", &directional->color.x);
                ImGui::DragFloat(
                    "Intensity",
                    &directional->intensity,
                    0.05F
                );
                directional->intensity =
                    std::max(directional->intensity, 0.0F);
            } else if (auto* point = std::get_if<
                    vshade::renderer::PointLight
                >(&component.light)) {
                ImGui::DragFloat3(
                    "Local Position",
                    &point->position.x,
                    0.1F
                );
                ImGui::ColorEdit3("Color", &point->color.x);
                ImGui::DragFloat("Intensity", &point->intensity, 0.05F);
                ImGui::DragFloat("Range", &point->range, 0.1F);
                point->intensity = std::max(point->intensity, 0.0F);
                point->range = std::max(point->range, 0.0F);
            }
        }
    );
}

void InspectorPanel::drawAudioListener(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::AudioListenerComponent>(
        "Audio Listener",
        entity,
        m_editHooks,
        [entity](vshade::scene::AudioListenerComponent& listener) {
            if (ImGui::Checkbox("Active", &listener.active) && listener.active) {
                activateExclusiveListener(entity);
            }
        }
    );
}

void InspectorPanel::drawRigidBody2D(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::RigidBody2DComponent>(
        "Rigid Body 2D",
        entity,
        m_editHooks,
        [](vshade::scene::RigidBody2DComponent& body) {
            auto& settings = body.settings;
            drawBodyType(settings.type);
            ImGui::DragFloat("Linear Damping", &settings.linearDamping, 0.01F);
            settings.linearDamping = std::max(settings.linearDamping, 0.0F);
            ImGui::DragFloat("Angular Damping", &settings.angularDamping, 0.01F);
            settings.angularDamping = std::max(settings.angularDamping, 0.0F);
            ImGui::DragFloat("Gravity Scale", &settings.gravityScale, 0.05F);
            ImGui::Checkbox("Fixed Rotation", &settings.fixedRotation);
            ImGui::Checkbox("Continuous Collision", &settings.continuousCollision);
            ImGui::Checkbox("Enabled", &settings.enabled);
        }
    );
}

void InspectorPanel::drawCollider2D(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::Collider2DComponent>(
        "Collider 2D",
        entity,
        m_editHooks,
        [](vshade::scene::Collider2DComponent& collider) {
            int shape = static_cast<int>(collider.shape.index());
            constexpr const char* shapeNames[] = {"Box", "Circle", "Capsule"};
            if (ImGui::Combo(
                    "Shape",
                    &shape,
                    shapeNames,
                    IM_ARRAYSIZE(shapeNames)
                )) {
                if (shape == 0) {
                    collider.shape = vshade::physics::BoxShape2D{};
                } else if (shape == 1) {
                    collider.shape = vshade::physics::CircleShape2D{};
                } else {
                    collider.shape = vshade::physics::CapsuleShape2D{};
                }
            }

            if (auto* box = std::get_if<vshade::physics::BoxShape2D>(
                    &collider.shape
                )) {
                ImGui::DragFloat2("Half Extents", &box->halfExtents.x, 0.05F);
                box->halfExtents.x = std::max(box->halfExtents.x, 0.001F);
                box->halfExtents.y = std::max(box->halfExtents.y, 0.001F);
            } else if (auto* circle = std::get_if<vshade::physics::CircleShape2D>(
                    &collider.shape
                )) {
                ImGui::DragFloat("Radius", &circle->radius, 0.05F);
                circle->radius = std::max(circle->radius, 0.001F);
            } else if (auto* capsule = std::get_if<vshade::physics::CapsuleShape2D>(
                    &collider.shape
                )) {
                ImGui::DragFloat("Half Height", &capsule->halfHeight, 0.05F);
                ImGui::DragFloat("Radius", &capsule->radius, 0.05F);
                capsule->halfHeight = std::max(capsule->halfHeight, 0.001F);
                capsule->radius = std::max(capsule->radius, 0.001F);
            }

            ImGui::DragFloat2("Offset", &collider.offset.x, 0.05F);
            ImGui::DragFloat("Density", &collider.material.density, 0.05F);
            ImGui::DragFloat("Friction", &collider.material.friction, 0.01F);
            ImGui::DragFloat("Restitution", &collider.material.restitution, 0.01F);
            collider.material.density = std::max(collider.material.density, 0.001F);
            collider.material.friction = std::max(collider.material.friction, 0.0F);
            collider.material.restitution =
                std::max(collider.material.restitution, 0.0F);
            ImGui::Checkbox("Sensor", &collider.sensor);
        }
    );
}

void InspectorPanel::drawRigidBody3D(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::RigidBody3DComponent>(
        "Rigid Body 3D",
        entity,
        m_editHooks,
        [](vshade::scene::RigidBody3DComponent& body) {
            auto& settings = body.settings;
            drawBodyType(settings.type);
            ImGui::DragFloat("Mass", &settings.mass, 0.05F);
            settings.mass = std::max(settings.mass, 0.001F);
            ImGui::DragFloat("Linear Damping", &settings.linearDamping, 0.01F);
            settings.linearDamping = std::max(settings.linearDamping, 0.0F);
            ImGui::DragFloat("Angular Damping", &settings.angularDamping, 0.01F);
            settings.angularDamping = std::max(settings.angularDamping, 0.0F);
            ImGui::DragFloat("Gravity Scale", &settings.gravityScale, 0.05F);
            ImGui::Checkbox("Continuous Collision", &settings.continuousCollision);
            ImGui::Checkbox("Enabled", &settings.enabled);
        }
    );
}

void InspectorPanel::drawCollider3D(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::Collider3DComponent>(
        "Collider 3D",
        entity,
        m_editHooks,
        [](vshade::scene::Collider3DComponent& collider) {
            int shape = static_cast<int>(collider.shape.index());
            constexpr const char* shapeNames[] = {"Box", "Sphere", "Capsule"};
            if (ImGui::Combo(
                    "Shape",
                    &shape,
                    shapeNames,
                    IM_ARRAYSIZE(shapeNames)
                )) {
                if (shape == 0) {
                    collider.shape = vshade::physics::BoxShape3D{};
                } else if (shape == 1) {
                    collider.shape = vshade::physics::SphereShape3D{};
                } else {
                    collider.shape = vshade::physics::CapsuleShape3D{};
                }
            }

            if (auto* box = std::get_if<vshade::physics::BoxShape3D>(
                    &collider.shape
                )) {
                ImGui::DragFloat3("Half Extents", &box->halfExtents.x, 0.05F);
                box->halfExtents.x = std::max(box->halfExtents.x, 0.001F);
                box->halfExtents.y = std::max(box->halfExtents.y, 0.001F);
                box->halfExtents.z = std::max(box->halfExtents.z, 0.001F);
            } else if (auto* sphere = std::get_if<vshade::physics::SphereShape3D>(
                    &collider.shape
                )) {
                ImGui::DragFloat("Radius", &sphere->radius, 0.05F);
                sphere->radius = std::max(sphere->radius, 0.001F);
            } else if (auto* capsule = std::get_if<vshade::physics::CapsuleShape3D>(
                    &collider.shape
                )) {
                ImGui::DragFloat("Half Height", &capsule->halfHeight, 0.05F);
                ImGui::DragFloat("Radius", &capsule->radius, 0.05F);
                capsule->halfHeight = std::max(capsule->halfHeight, 0.001F);
                capsule->radius = std::max(capsule->radius, 0.001F);
            }

            ImGui::DragFloat3("Offset", &collider.offset.x, 0.05F);
            ImGui::DragFloat("Friction", &collider.material.friction, 0.01F);
            ImGui::DragFloat("Restitution", &collider.material.restitution, 0.01F);
            collider.material.friction = std::max(collider.material.friction, 0.0F);
            collider.material.restitution =
                std::max(collider.material.restitution, 0.0F);
            ImGui::Checkbox("Sensor", &collider.sensor);
        }
    );
}

void InspectorPanel::drawTypedColliders3D(vshade::scene::Entity entity) {
    const auto fitButton = [this, entity](auto& collider) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Fit");
        ImGui::SameLine();
        if (ui::iconButton(
                "##ResetToModelBounds",
                ui::Icon::Reset,
                ui::scaled(26.0F, 24.0F),
                "Reset collider to the model's local bounds"
            )) {
            commitMutation(m_editHooks, [&] {
                if (!fitColliderToModel(entity, m_assets, collider)) {
                    ENGINE_WARN(
                        "Entity '{}' has no loaded model bounds to fit",
                        entity.name()
                    );
                }
            });
        }
    };

    drawComponent<vshade::scene::BoxCollider3DComponent>(
        "Box Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            ImGui::DragFloat3(
                "Half Extents",
                &collider.halfExtents.x,
                0.05F
            );
            collider.halfExtents = glm::max(
                collider.halfExtents,
                vshade::math::Vec3{0.001F}
            );
            drawColliderProperties(collider);
        }
    );
    drawComponent<vshade::scene::SphereCollider3DComponent>(
        "Sphere Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            ImGui::DragFloat("Radius", &collider.radius, 0.05F);
            collider.radius = std::max(collider.radius, 0.001F);
            drawColliderProperties(collider);
        }
    );
    drawComponent<vshade::scene::CapsuleCollider3DComponent>(
        "Capsule Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            ImGui::DragFloat("Half Height", &collider.halfHeight, 0.05F);
            ImGui::DragFloat("Radius", &collider.radius, 0.05F);
            collider.halfHeight = std::max(collider.halfHeight, 0.001F);
            collider.radius = std::max(collider.radius, 0.001F);
            drawColliderProperties(collider);
        }
    );
    drawComponent<vshade::scene::CylinderCollider3DComponent>(
        "Cylinder Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            ImGui::DragFloat("Half Height", &collider.halfHeight, 0.05F);
            ImGui::DragFloat("Radius", &collider.radius, 0.05F);
            collider.halfHeight = std::max(collider.halfHeight, 0.001F);
            collider.radius = std::max(collider.radius, 0.001F);
            drawColliderProperties(collider);
        }
    );
    drawComponent<vshade::scene::MeshCollider3DComponent>(
        "Mesh Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            if (m_assets && m_assetSelector) {
                m_assetSelector->draw("Model", collider.model, *m_assets);
            }
            ImGui::TextDisabled("Static rigid bodies only");
            drawColliderProperties(collider);
        }
    );
    drawComponent<vshade::scene::ConvexCollider3DComponent>(
        "Convex Collider 3D",
        entity,
        m_editHooks,
        [&](auto& collider) {
            fitButton(collider);
            if (m_assets && m_assetSelector) {
                m_assetSelector->draw("Model", collider.model, *m_assets);
            }
            drawColliderProperties(collider);
        }
    );
}

void InspectorPanel::drawScripts(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::ScriptComponent>(
        "Scripts",
        entity,
        m_editHooks,
        [this](vshade::scene::ScriptComponent& component) {
            const std::vector<std::string> typeNames = m_scripts
                ? m_scripts->typeNames()
                : std::vector<std::string>{};

            int removeIndex = -1;
            for (std::size_t index = 0; index < component.scripts.size(); ++index) {
                auto& binding = component.scripts[index];
                ImGui::PushID(static_cast<int>(index));
                ImGui::Checkbox("##Enabled", &binding.enabled);
                ImGui::SameLine();

                const std::string preview = binding.typeName.empty()
                    ? std::string("<None>")
                    : binding.typeName;
                if (!typeNames.empty()) {
                    if (ImGui::BeginCombo("Type", preview.c_str())) {
                        for (const std::string& name : typeNames) {
                            const bool selected = binding.typeName == name;
                            if (ImGui::Selectable(name.c_str(), selected)) {
                                binding.typeName = name;
                            }
                            if (selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                } else {
                    std::array<char, 256> buffer{};
                    const std::size_t characterCount = std::min(
                        binding.typeName.size(),
                        buffer.size() - 1
                    );
                    std::memcpy(buffer.data(), binding.typeName.data(), characterCount);
                    if (ImGui::InputText("Type", buffer.data(), buffer.size())) {
                        binding.typeName = buffer.data();
                    }
                }

                ImGui::SameLine();
                if (ImGui::SmallButton("Remove")) {
                    removeIndex = static_cast<int>(index);
                }
                const bool missingScript = binding.typeName.empty()
                    || m_scripts == nullptr
                    || !m_scripts->contains(binding.typeName);
                if (missingScript) {
                    ImGui::TextColored(
                        ui::color(ui::ColorRole::Warning),
                        "Missing script: %s",
                        binding.typeName.empty()
                            ? "no type selected"
                            : binding.typeName.c_str()
                    );
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip(
                            "This script type is not registered and will not run."
                        );
                    }
                }
                ImGui::PopID();
            }

            if (removeIndex >= 0) {
                component.scripts.erase(
                    component.scripts.begin() + removeIndex
                );
            }

            if (ImGui::Button("Add Script")) {
                vshade::scene::ScriptBinding binding;
                if (!typeNames.empty()) {
                    binding.typeName = typeNames.front();
                }
                component.scripts.push_back(std::move(binding));
            }
        }
    );
}

void InspectorPanel::drawAddComponentMenu(
    vshade::scene::Entity entity
) {
    if (ImGui::Button("+ Add Component", {-1.0F, 0.0F})) {
        ImGui::OpenPopup("AddComponent");
        m_componentSearch.fill('\0');
    }

    if (!ImGui::BeginPopup("AddComponent")) {
        return;
    }

    ImGui::SetNextItemWidth(ui::scaled(280.0F));
    ImGui::InputTextWithHint(
        "##ComponentSearch",
        "Search components...",
        m_componentSearch.data(),
        m_componentSearch.size()
    );
    ImGui::Separator();

    std::string query = m_componentSearch.data();
    std::ranges::transform(query, query.begin(), [](const unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    const auto matches = [&query](std::string_view name) {
        std::string candidate(name);
        std::ranges::transform(
            candidate,
            candidate.begin(),
            [](const unsigned char value) {
                return static_cast<char>(std::tolower(value));
            }
        );
        return query.empty() || candidate.find(query) != std::string::npos;
    };
    const auto group = [](const char* name) {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", name);
    };
    const auto close = [] { ImGui::CloseCurrentPopup(); };

    group("Rendering");

    if (!entity.has<vshade::scene::CameraComponent>()
        && matches("Camera") && ImGui::MenuItem("Camera")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::CameraComponent>();
        });
        close();
    }
    if (!entity.has<vshade::scene::SpriteRendererComponent>()
        && matches("Sprite Renderer") && ImGui::MenuItem("Sprite Renderer")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::SpriteRendererComponent>();
        });
        close();
    }
    if (!entity.has<vshade::scene::ModelRendererComponent>()
        && matches("Model Renderer") && ImGui::MenuItem("Model Renderer")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::ModelRendererComponent>();
        });
        close();
    }
    if (!entity.has<vshade::scene::LightComponent>()
        && matches("Directional Light") && ImGui::MenuItem("Directional Light")) {
        commitMutation(m_editHooks, [&] {
            vshade::scene::LightComponent light;
            light.light = vshade::renderer::DirectionalLight{};
            entity.add<vshade::scene::LightComponent>(light);
        });
        close();
    }
    if (!entity.has<vshade::scene::LightComponent>()
        && matches("Point Light") && ImGui::MenuItem("Point Light")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::LightComponent>();
        });
        close();
    }

    group("Audio");
    if (!entity.has<vshade::scene::AudioSourceComponent>()
        && matches("Audio Source") && ImGui::MenuItem("Audio Source")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::AudioSourceComponent>();
        });
        close();
    }
    if (!entity.has<vshade::scene::AudioListenerComponent>()
        && matches("Audio Listener") && ImGui::MenuItem("Audio Listener")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::AudioListenerComponent>();
            activateExclusiveListener(entity);
        });
        close();
    }

    group("Physics");
    if (!entity.has<vshade::scene::RigidBody2DComponent>()
        && matches("Rigid Body 2D") && ImGui::MenuItem("Rigid Body 2D")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::RigidBody2DComponent>();
            if (!entity.has<vshade::scene::Collider2DComponent>()) {
                entity.add<vshade::scene::Collider2DComponent>();
            }
        });
        close();
    }
    if (entity.has<vshade::scene::RigidBody2DComponent>()
        && !entity.has<vshade::scene::Collider2DComponent>()
        && matches("Collider 2D") && ImGui::MenuItem("Collider 2D")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::Collider2DComponent>();
        });
        close();
    }
    if (!entity.has<vshade::scene::RigidBody3DComponent>()
        && matches("Rigid Body 3D") && ImGui::MenuItem("Rigid Body 3D")) {
        commitMutation(m_editHooks, [&] {
            entity.add<vshade::scene::RigidBody3DComponent>();
        });
        close();
    }
    if (!hasAnyCollider3D(entity)) {
        const auto addCollider = [&]<typename Component>() {
            commitMutation(m_editHooks, [&] {
                if (!entity.has<vshade::scene::RigidBody3DComponent>()) {
                    entity.add<vshade::scene::RigidBody3DComponent>();
                }
                auto& collider = entity.add<Component>();
                (void)fitColliderToModel(entity, m_assets, collider);
            });
            close();
        };
        if (matches("Box Collider 3D") && ImGui::MenuItem("Box Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::BoxCollider3DComponent>();
        }
        if (matches("Sphere Collider 3D") && ImGui::MenuItem("Sphere Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::SphereCollider3DComponent>();
        }
        if (matches("Capsule Collider 3D") && ImGui::MenuItem("Capsule Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::CapsuleCollider3DComponent>();
        }
        if (matches("Cylinder Collider 3D") && ImGui::MenuItem("Cylinder Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::CylinderCollider3DComponent>();
        }
        if (matches("Mesh Collider 3D") && ImGui::MenuItem("Mesh Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::MeshCollider3DComponent>();
        }
        if (matches("Convex Collider 3D") && ImGui::MenuItem("Convex Collider 3D")) {
            addCollider.template operator()<
                vshade::scene::ConvexCollider3DComponent>();
        }
    }

    group("Scripting");
    if (!entity.has<vshade::scene::ScriptComponent>()
        && matches("Script") && ImGui::MenuItem("Script")) {
        commitMutation(m_editHooks, [&] {
            vshade::scene::ScriptComponent scripts;
            if (m_scripts) {
                const auto typeNames = m_scripts->typeNames();
                if (!typeNames.empty()) {
                    scripts.scripts.push_back({
                        .typeName = typeNames.front(),
                    });
                }
            }
            entity.add<vshade::scene::ScriptComponent>(std::move(scripts));
        });
        close();
    }

    group("Core");
    ImGui::TextDisabled("Transform and identity are required components.");

    ImGui::EndPopup();
}

} // namespace editor
