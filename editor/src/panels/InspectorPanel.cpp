#include "panels/InspectorPanel.hpp"

#include <math/Quaternion.hpp>
#include <scene/components/CoreComponents.hpp>

#include <algorithm>
#include <array>
#include <cstring>

#include <glm/trigonometric.hpp>
#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

} // namespace

void InspectorPanel::onImGuiRender(
    vshade::scene::Entity selectedEntity
) {
    ImGui::Begin("Inspector", nullptr, panelFlags);

    if (selectedEntity) {
        drawTag(selectedEntity);
        ImGui::Separator();
        drawTransform(selectedEntity);
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

} // namespace editor
