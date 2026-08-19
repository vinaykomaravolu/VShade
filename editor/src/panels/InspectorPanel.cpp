#include "panels/InspectorPanel.hpp"
#include "widgets/AssetSelector.hpp"

#include <audio/AudioClip.hpp>
#include <math/Quaternion.hpp>
#include <renderer/Lighting.hpp>
#include <scene/components/AudioComponents.hpp>
#include <scene/components/CoreComponents.hpp>
#include <scene/components/LightComponent.hpp>
#include <scene/components/RenderComponents.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>
#include <variant>

#include <glm/trigonometric.hpp>
#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

template<typename Component, typename UiFunction, typename DropFunction>
void drawComponent(
    const char* name,
    vshade::scene::Entity entity,
    UiFunction&& uiFunction,
    DropFunction&& dropFunction
) {
    if (!entity.has<Component>()) {
        return;
    }

    auto& component = entity.get<Component>();
    ImGui::PushID(name);
    const bool open = ImGui::CollapsingHeader(
        name,
        ImGuiTreeNodeFlags_DefaultOpen
    );
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
    UiFunction&& uiFunction
) {
    drawComponent<Component>(
        name,
        entity,
        std::forward<UiFunction>(uiFunction),
        [](Component&) {}
    );
}

} // namespace

void InspectorPanel::setAssetManager(
    vshade::asset::AssetManager& assets
) noexcept {
    m_assets = &assets;
}

void InspectorPanel::onImGuiRender(
    vshade::scene::Entity selectedEntity
) {
    ImGui::Begin("Inspector", nullptr, panelFlags);

    if (selectedEntity) {
        drawTag(selectedEntity);
        ImGui::Separator();
        drawTransform(selectedEntity);
        drawCamera(selectedEntity);
        drawSpriteRenderer(selectedEntity);
        drawModelRenderer(selectedEntity);
        drawAudioSource(selectedEntity);
        drawLight(selectedEntity);
        ImGui::Separator();
        drawAddComponentMenu(selectedEntity);
    } else {
        ImGui::TextDisabled("No entity selected");
    }

    ImGui::End();
}

void InspectorPanel::drawTag(vshade::scene::Entity entity) {
    if (!entity.has<vshade::scene::TagComponent>()) {
        return;
    }

    std::array<char, 256> buffer{};
    const std::string_view name = entity.name();
    const std::size_t characterCount =
        std::min(name.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), name.data(), characterCount);

    if (ImGui::InputText("Name", buffer.data(), buffer.size())) {
        entity.setName(buffer.data());
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
    if (ImGui::DragFloat3("Position", &position.x, 0.1F)) {
        transform.setPosition(position);
    }

    vshade::math::Vec3 rotationDegrees =
        glm::degrees(vshade::math::toEuler(transform.rotation()));
    if (ImGui::DragFloat3("Rotation", &rotationDegrees.x, 0.5F)) {
        transform.setRotation(
            vshade::math::fromEuler(glm::radians(rotationDegrees))
        );
    }

    vshade::math::Vec3 scale = transform.scale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1F)) {
        constexpr float minimumScale = 0.001F;
        scale.x = std::max(scale.x, minimumScale);
        scale.y = std::max(scale.y, minimumScale);
        scale.z = std::max(scale.z, minimumScale);
        transform.setScale(scale);
    }
}

void InspectorPanel::drawCamera(vshade::scene::Entity entity) {
    drawComponent<vshade::scene::CameraComponent>(
        "Camera",
        entity,
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
                        179.0F
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
                    0.001F
                );
                camera.orthographicHeight =
                    std::max(camera.orthographicHeight, 0.001F);
            }

            ImGui::DragFloat("Near Clip", &camera.nearPlane, 0.01F);
            camera.nearPlane = std::max(camera.nearPlane, 0.001F);
            ImGui::DragFloat("Far Clip", &camera.farPlane, 1.0F);
            camera.farPlane = std::max(
                camera.farPlane,
                camera.nearPlane + 0.001F
            );
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
        [this](vshade::scene::SpriteRendererComponent& sprite) {
            if (m_assets
                && AssetSelector::draw(
                    "Texture",
                    sprite.texture,
                    *m_assets
                )) {
                sprite.texturePath = sprite.texture
                    ? sprite.texture.sourcePath()
                    : std::filesystem::path{};
            }
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
        [this](vshade::scene::ModelRendererComponent& modelRenderer) {
            if (m_assets) {
                AssetSelector::draw(
                    "Model",
                    modelRenderer.model,
                    *m_assets
                );
            }
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
        [this](vshade::scene::AudioSourceComponent& source) {
            if (m_assets
                && AssetSelector::draw(
                    "Clip",
                    source.clipAsset,
                    *m_assets
                )) {
                source.clip = source.clipAsset
                    ? source.clipAsset.handle()
                    : vshade::audio::AudioClipHandle{};
            }
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

void InspectorPanel::drawAddComponentMenu(
    vshade::scene::Entity entity
) {
    if (ImGui::Button("+ Add Component")) {
        ImGui::OpenPopup("AddComponent");
    }

    if (!ImGui::BeginPopup("AddComponent")) {
        return;
    }

    if (!entity.has<vshade::scene::CameraComponent>()
        && ImGui::MenuItem("Camera")) {
        entity.add<vshade::scene::CameraComponent>();
        ImGui::CloseCurrentPopup();
    }
    if (!entity.has<vshade::scene::SpriteRendererComponent>()
        && ImGui::MenuItem("Sprite Renderer")) {
        entity.add<vshade::scene::SpriteRendererComponent>();
        ImGui::CloseCurrentPopup();
    }
    if (!entity.has<vshade::scene::ModelRendererComponent>()
        && ImGui::MenuItem("Model Renderer")) {
        entity.add<vshade::scene::ModelRendererComponent>();
        ImGui::CloseCurrentPopup();
    }
    if (!entity.has<vshade::scene::AudioSourceComponent>()
        && ImGui::MenuItem("Audio Source")) {
        entity.add<vshade::scene::AudioSourceComponent>();
        ImGui::CloseCurrentPopup();
    }
    if (!entity.has<vshade::scene::LightComponent>()
        && ImGui::MenuItem("Directional Light")) {
        vshade::scene::LightComponent light;
        light.light = vshade::renderer::DirectionalLight{};
        entity.add<vshade::scene::LightComponent>(light);
        ImGui::CloseCurrentPopup();
    }
    if (!entity.has<vshade::scene::LightComponent>()
        && ImGui::MenuItem("Point Light")) {
        entity.add<vshade::scene::LightComponent>();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

} // namespace editor
