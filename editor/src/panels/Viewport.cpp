#include "panels/Viewport.hpp"
#include "EditorIcons.hpp"
#include "ImGui/ImGuiTheme.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <core/Log.hpp>
#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <input/MouseCode.hpp>
#include <math/Quaternion.hpp>
#include <math/Ray.hpp>
#include <math/Transform.hpp>
#include <math/Vector.hpp>
#include <renderer/DebugDraw.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Model.hpp>
#include <renderer/Renderer.hpp>
#include <renderer/Texture.hpp>
#include <scene/Prefab.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneRenderer.hpp>
#include <scene/SceneRuntime.hpp>
#include <scene/components/CoreComponents.hpp>
#include <scene/components/AudioComponents.hpp>
#include <scene/components/LightComponent.hpp>
#include <scene/components/RenderComponents.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

ImGuizmo::OPERATION toImGuizmoOperation(
    const Viewport::GizmoOperation operation
) {
    switch (operation) {
        case Viewport::GizmoOperation::Translate:
            return ImGuizmo::TRANSLATE;
        case Viewport::GizmoOperation::Rotate:
            return ImGuizmo::ROTATE;
        case Viewport::GizmoOperation::Scale:
            return ImGuizmo::SCALE;
        case Viewport::GizmoOperation::None:
            return ImGuizmo::TRANSLATE;
    }
    return ImGuizmo::TRANSLATE;
}

struct ViewportCursor {
    std::uint32_t pixelX = 0;
    std::uint32_t pixelY = 0;
    float ndcX = 0.0F;
    float ndcY = 0.0F;
};

[[nodiscard]] std::optional<ViewportCursor> cursorInViewport(
    const float x,
    const float y,
    const float width,
    const float height,
    const std::uint32_t framebufferWidth,
    const std::uint32_t framebufferHeight
) {
    if (width <= 0.0F || height <= 0.0F
        || framebufferWidth == 0 || framebufferHeight == 0) {
        return std::nullopt;
    }

    const ImVec2 mouse = ImGui::GetMousePos();
    const float localX = mouse.x - x;
    const float localY = mouse.y - y;
    if (localX < 0.0F || localY < 0.0F
        || localX >= width || localY >= height) {
        return std::nullopt;
    }

    ViewportCursor cursor;
    cursor.pixelX = std::min(
        static_cast<std::uint32_t>(
            localX / width * static_cast<float>(framebufferWidth)
        ),
        framebufferWidth - 1
    );
    const auto topDownPixelY = std::min(
        static_cast<std::uint32_t>(
            localY / height * static_cast<float>(framebufferHeight)
        ),
        framebufferHeight - 1
    );
    cursor.pixelY = framebufferHeight - 1 - topDownPixelY;
    cursor.ndcX = localX / width * 2.0F - 1.0F;
    cursor.ndcY = 1.0F - localY / height * 2.0F;
    return cursor;
}

[[nodiscard]] std::string assetStemName(
    const std::filesystem::path& path,
    const char* fallback
) {
    std::string name = path.stem().generic_string();
    if (name.empty()) {
        return fallback;
    }
    return name;
}

[[nodiscard]] std::optional<ImVec2> projectToViewport(
    const vshade::math::Vec3& worldPosition,
    const vshade::math::Mat4& viewProjection,
    const float x,
    const float y,
    const float width,
    const float height
) {
    const vshade::math::Vec4 clip = viewProjection
        * vshade::math::Vec4{worldPosition, 1.0F};
    if (clip.w <= 0.0F || !std::isfinite(clip.w)) {
        return std::nullopt;
    }
    const vshade::math::Vec3 ndc = vshade::math::Vec3{clip} / clip.w;
    if (ndc.x < -1.0F || ndc.x > 1.0F
        || ndc.y < -1.0F || ndc.y > 1.0F
        || ndc.z < -1.0F || ndc.z > 1.0F) {
        return std::nullopt;
    }
    return ImVec2{
        x + (ndc.x * 0.5F + 0.5F) * width,
        y + (0.5F - ndc.y * 0.5F) * height,
    };
}

void drawCameraSceneIcon(
    ImDrawList& drawList,
    const ImVec2 center,
    const ImU32 color,
    const float scale
) {
    const float halfWidth = 7.0F * scale;
    const float halfHeight = 5.0F * scale;
    drawList.AddRectFilled(
        {center.x - halfWidth, center.y - halfHeight},
        {center.x + 2.0F * scale, center.y + halfHeight},
        color,
        2.0F * scale
    );
    drawList.AddTriangleFilled(
        {center.x + 2.0F * scale, center.y - 3.5F * scale},
        {center.x + halfWidth, center.y - 6.0F * scale},
        {center.x + halfWidth, center.y + 6.0F * scale},
        color
    );
}

void drawPointLightSceneIcon(
    ImDrawList& drawList,
    const ImVec2 center,
    const ImU32 color,
    const float scale
) {
    drawList.AddCircleFilled(
        {center.x, center.y - 2.0F * scale},
        5.0F * scale,
        color,
        20
    );
    drawList.AddRectFilled(
        {center.x - 3.0F * scale, center.y + 3.0F * scale},
        {center.x + 3.0F * scale, center.y + 7.0F * scale},
        color,
        scale
    );
    drawList.AddLine(
        {center.x - 2.5F * scale, center.y + 9.0F * scale},
        {center.x + 2.5F * scale, center.y + 9.0F * scale},
        color,
        1.5F * scale
    );
}

void drawDirectionalLightSceneIcon(
    ImDrawList& drawList,
    const ImVec2 center,
    const ImU32 color,
    const float scale
) {
    const float innerRadius = 5.0F * scale;
    drawList.AddCircleFilled(center, innerRadius, color, 24);
    constexpr float twoPi = 6.283185307F;
    for (int ray = 0; ray < 8; ++ray) {
        const float angle = twoPi * static_cast<float>(ray) / 8.0F;
        const vshade::math::Vec2 direction{std::cos(angle), std::sin(angle)};
        drawList.AddLine(
            {
                center.x + direction.x * 7.0F * scale,
                center.y + direction.y * 7.0F * scale,
            },
            {
                center.x + direction.x * 10.0F * scale,
                center.y + direction.y * 10.0F * scale,
            },
            color,
            1.6F * scale
        );
    }
}

void drawSpeakerSceneIcon(
    ImDrawList& drawList,
    const ImVec2 center,
    const ImU32 color,
    const float scale
) {
    drawList.AddRectFilled(
        {center.x - 7.0F * scale, center.y - 3.0F * scale},
        {center.x - 3.0F * scale, center.y + 3.0F * scale},
        color,
        scale
    );
    drawList.AddTriangleFilled(
        {center.x - 3.0F * scale, center.y - 4.0F * scale},
        {center.x + 2.0F * scale, center.y - 7.0F * scale},
        {center.x + 2.0F * scale, center.y + 7.0F * scale},
        color
    );
    drawList.PathArcTo(
        {center.x + 1.0F * scale, center.y},
        6.0F * scale,
        -0.8F,
        0.8F,
        12
    );
    drawList.PathStroke(color, 0, 1.6F * scale);
}

[[nodiscard]] bool drawTextureSceneIcon(
    ImDrawList& drawList,
    const std::shared_ptr<vshade::renderer::Texture2D>& texture,
    const ImVec2 center,
    const float scale,
    const float alpha = 1.0F
) {
    if (!texture) {
        return false;
    }
    const float extent = 9.0F * scale;
    const ImTextureID textureId =
        static_cast<ImTextureID>(texture->rendererId());
    drawList.AddImage(
        ImTextureRef{textureId},
        {center.x - extent, center.y - extent},
        {center.x + extent, center.y + extent},
        {0.0F, 0.0F},
        {1.0F, 1.0F},
        ImGui::ColorConvertFloat4ToU32({1.0F, 1.0F, 1.0F, alpha})
    );
    return true;
}

} // namespace

Viewport::Viewport(vshade::asset::AssetManager& assets)
    : m_framebuffer(
          std::make_unique<vshade::renderer::Framebuffer>(1280, 720)
      ),
      m_sceneRenderer(
          std::make_unique<vshade::scene::SceneRenderer>(assets)
      ),
      m_assets(&assets),
      m_editorCamera(0.785398163F, 16.0F / 9.0F, 0.1F, 1000.0F) {}

Viewport::~Viewport() = default;

bool Viewport::drawOverlayToolbar(
    const vshade::math::Vec2 position,
    const vshade::math::Vec2 size
) {
    using ui::Metrics;
    const ImVec2 toolSize = ui::scaled(30.0F, Metrics::controlHeight);
    const float padding = ui::scaled(10.0F);
    const float cardPadding = ui::scaled(6.0F);
    const float spacing = ui::scaled(Metrics::smallSpacing);
    ImVec4 overlayColor = ui::color(ui::ColorRole::Chrome);
    overlayColor.w = 0.88F;
    constexpr ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoDocking
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoNav;
    bool overlayHovered = false;
    const ImGuiID viewportId = ImGui::GetWindowViewport()->ID;

    const auto toolButton = [toolSize](
        const char* id,
        const ui::Icon icon,
        const char* tooltip,
        const bool active
    ) {
        return ui::iconButton(id, icon, toolSize, tooltip, active);
    };

    const ImVec2 toolsCardSize{
        toolSize.x + cardPadding * 2.0F,
        toolSize.y * 3.0F + spacing * 2.0F + cardPadding * 2.0F,
    };
    ImGui::SetNextWindowPos({position.x + padding, position.y + padding});
    ImGui::SetNextWindowSize(toolsCardSize);
    ImGui::SetNextWindowViewport(viewportId);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, overlayColor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ui::scaled(8.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0F);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2{cardPadding, cardPadding}
    );
    ImGui::Begin("##ViewportTransformTools", nullptr, overlayFlags);
    if (toolButton("##Translate", ui::Icon::Translate, "Translate (W)", m_gizmoOperation == GizmoOperation::Translate)) {
        m_gizmoOperation = GizmoOperation::Translate;
    }
    if (toolButton("##Rotate", ui::Icon::Rotate, "Rotate (E)", m_gizmoOperation == GizmoOperation::Rotate)) {
        m_gizmoOperation = GizmoOperation::Rotate;
    }
    if (toolButton("##Scale", ui::Icon::Scale, "Scale (R)", m_gizmoOperation == GizmoOperation::Scale)) {
        m_gizmoOperation = GizmoOperation::Scale;
    }
    overlayHovered |= ImGui::IsWindowHovered();
    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();

    const ImVec2 optionsPadding = ui::scaled(8.0F, 5.0F);
    const ImVec2 optionFramePadding = ui::scaled(10.0F, 3.0F);
    const float optionsWidth = ui::scaled(m_snapEnabled ? 500.0F : 415.0F);
    const float optionsHeight = ImGui::GetFontSize()
        + optionFramePadding.y * 2.0F
        + optionsPadding.y * 2.0F;
    if (size.x < optionsWidth + toolsCardSize.x + padding * 3.0F) {
        return overlayHovered;
    }

    ImGui::SetNextWindowPos({
        position.x + size.x - optionsWidth - padding,
        position.y + padding,
    });
    ImGui::SetNextWindowSize({optionsWidth, optionsHeight});
    ImGui::SetNextWindowViewport(viewportId);
    ImVec4 overlayBorder = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    overlayBorder.w = 0.65F;
    ImGui::PushStyleColor(ImGuiCol_WindowBg, overlayColor);
    ImGui::PushStyleColor(ImGuiCol_Border, overlayBorder);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ui::scaled(8.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, optionsPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, optionFramePadding);
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2{spacing, ui::scaled(2.0F)}
    );
    ImGui::Begin("##ViewportTransformOptions", nullptr, overlayFlags);
    ImGui::SetNextItemWidth(ui::scaled(92.0F));
    const char* orientation = m_gizmoOrientation == GizmoOrientation::Local
        ? "Local"
        : "World";
    if (ImGui::BeginCombo("##GizmoOrientation", orientation)) {
        if (ImGui::Selectable("Local", m_gizmoOrientation == GizmoOrientation::Local)) {
            m_gizmoOrientation = GizmoOrientation::Local;
        }
        if (ImGui::Selectable("World", m_gizmoOrientation == GizmoOrientation::World)) {
            m_gizmoOrientation = GizmoOrientation::World;
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Gizmo orientation");
    }

    ImGui::SameLine(0.0F, spacing);
    ImGui::SetNextItemWidth(ui::scaled(112.0F));
    const char* pivot = m_pivotMode == PivotMode::Object
        ? "Pivot: Object"
        : "Pivot: Selection";
    if (ImGui::BeginCombo("##PivotMode", pivot)) {
        if (ImGui::Selectable("Object", m_pivotMode == PivotMode::Object)) {
            m_pivotMode = PivotMode::Object;
        }
        if (ImGui::Selectable("Selection", m_pivotMode == PivotMode::Selection)) {
            m_pivotMode = PivotMode::Selection;
        }
        ImGui::EndCombo();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Transform pivot mode (selection center is used for multi-selection)");
    }

    ImGui::SameLine(0.0F, ui::scaled(Metrics::spacing));
    ImGui::Checkbox("Snap", &m_snapEnabled);
    if (m_snapEnabled) {
        ImGui::SameLine(0.0F, spacing);
        ImGui::SetNextItemWidth(ui::scaled(72.0F));
        float* snapValue = &m_translationSnap;
        const char* snapFormat = "%.2f";
        float minimum = 0.01F;
        if (m_gizmoOperation == GizmoOperation::Rotate) {
            snapValue = &m_rotationSnap;
            snapFormat = "%.0f deg";
            minimum = 1.0F;
        } else if (m_gizmoOperation == GizmoOperation::Scale) {
            snapValue = &m_scaleSnap;
        }
        ImGui::DragFloat(
            "##SnapValue",
            snapValue,
            minimum,
            minimum,
            100.0F,
            snapFormat
        );
    }
    ImGui::SameLine(0.0F, spacing);
    ImGui::SetNextItemWidth(ui::scaled(88.0F));
    if (ImGui::BeginCombo("##GizmoVisibility", "Gizmos")) {
        ImGui::MenuItem("Cameras", nullptr, &m_showCameraGizmos);
        ImGui::MenuItem("Lights", nullptr, &m_showLightGizmos);
        ImGui::MenuItem("Colliders", nullptr, &m_showColliderGizmos);
        ImGui::MenuItem("Audio", nullptr, &m_showAudioGizmos);
        ImGui::Separator();
        ImGui::MenuItem("Physics", nullptr, &m_showPhysicsGizmos);
        ImGui::EndCombo();
    }
    overlayHovered |= ImGui::IsWindowHovered();
    ImGui::End();
    ImGui::PopStyleVar(5);
    ImGui::PopStyleColor(2);
    return overlayHovered;
}

void Viewport::onUpdate(const float deltaTime) {
    using vshade::input::Input;
    using vshade::input::KeyCode;
    using vshade::input::MouseButton;

    const bool cameraLookActive =
        Input::isMouseButtonDown(MouseButton::Right);
    if (m_editing
        && m_visible
        && !ImGui::GetIO().WantTextInput
        && Input::isKeyPressed(KeyCode::F)
        && m_scene
        && m_scene->valid(m_selectedEntity)) {
        const vshade::math::Transform& transform = m_selectedEntity.transform();
        const vshade::math::Vec3 scale = transform.scale();
        const float largestAxis = std::max({
            std::abs(scale.x),
            std::abs(scale.y),
            std::abs(scale.z),
            0.25F,
        });
        constexpr float unitCubeRadius = 0.866025404F;
        m_editorCamera.focusOn(
            transform.position(),
            largestAxis * unitCubeRadius
        );
    }
    if (m_editing && m_visible && m_hovered && !m_gizmoUsing && !cameraLookActive) {
        if (Input::isKeyPressed(KeyCode::W)) {
            m_gizmoOperation = GizmoOperation::Translate;
        } else if (Input::isKeyPressed(KeyCode::E)) {
            m_gizmoOperation = GizmoOperation::Rotate;
        } else if (Input::isKeyPressed(KeyCode::R)) {
            m_gizmoOperation = GizmoOperation::Scale;
        }
    }

    m_editorCamera.setInputEnabled(
        m_editing
        && m_visible
        && m_hovered
        && !m_overlayHovered
        && !m_gizmoUsing
    );
    m_editorCamera.onUpdate(deltaTime);
}

void Viewport::setScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    m_scene = std::move(scene);
}

void Viewport::setRuntime(vshade::scene::SceneRuntime* runtime) noexcept {
    m_runtime = runtime;
}

void Viewport::setEditing(const bool editing) noexcept {
    m_editing = editing;
    if (!editing) {
        m_gizmoUsing = false;
        m_overlayHovered = false;
        m_editorCamera.setInputEnabled(false);
    }
}

void Viewport::setVisible(const bool visible) noexcept {
    m_visible = visible;
    if (!visible) {
        m_hovered = false;
        m_overlayHovered = false;
        m_gizmoUsing = false;
    }
}

void Viewport::setEditHooks(SceneEditHooks hooks) {
    m_editHooks = std::move(hooks);
}

void Viewport::finishGizmoRecording() {
    if (!m_gizmoRecording) {
        return;
    }
    if (m_editHooks.commit) {
        m_editHooks.commit();
    }
    m_gizmoRecording = false;
}

void Viewport::onImGuiRender(
    vshade::scene::Entity& selectedEntity
) {
    const bool visible = ImGui::Begin("Viewport", nullptr, panelFlags);
    m_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    if (visible) {
        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        resizeFramebuffer(availableSize.x, availableSize.y);
        m_editorCamera.setViewportSize(availableSize.x, availableSize.y);
        if (m_editing && m_showCameraGizmos && availableSize.y > 0.0F) {
            queueSelectedCameraFrustum(
                selectedEntity,
                availableSize.x / availableSize.y
            );
        }
        if (m_editing && m_showColliderGizmos) {
            queueSelectedColliderGizmo(selectedEntity);
        }
        if (m_editing && m_showPhysicsGizmos) {
            queuePhysicsGizmos();
        }
        renderScene();

        const ImVec2 viewportPosition = ImGui::GetCursorScreenPos();
        const ImTextureID textureId =
            static_cast<ImTextureID>(m_framebuffer->colorAttachmentId());
        ImGui::Image(
            ImTextureRef{textureId},
            availableSize,
            {0.0F, 1.0F},
            {1.0F, 0.0F}
        );
        const bool imageHovered = ImGui::IsItemHovered();
        if (m_editing) {
            const bool spawnedAsset = spawnDroppedAsset(
                selectedEntity,
                viewportPosition.x,
                viewportPosition.y,
                availableSize.x,
                availableSize.y
            );
            m_overlayHovered = drawOverlayToolbar(
                {viewportPosition.x, viewportPosition.y},
                {availableSize.x, availableSize.y}
            );
            const bool sceneIconClicked = drawSceneIcons(
                selectedEntity,
                viewportPosition.x,
                viewportPosition.y,
                availableSize.x,
                availableSize.y,
                imageHovered && !m_overlayHovered
            );
            drawGizmo(
                selectedEntity,
                viewportPosition.x,
                viewportPosition.y,
                availableSize.x,
                availableSize.y
            );
            if (!spawnedAsset
                && !sceneIconClicked
                && !m_overlayHovered
                && imageHovered
                && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && !ImGuizmo::IsOver()
                && ImGui::GetDragDropPayload() == nullptr) {
                selectEntityUnderMouse(
                    selectedEntity,
                    viewportPosition.x,
                    viewportPosition.y,
                    availableSize.x,
                    availableSize.y
                );
            }
        } else {
            m_overlayHovered = false;
            finishGizmoRecording();
            m_gizmoUsing = false;
            if (!m_runtimeCameraActive) {
                const ImVec2 textSize = ImGui::CalcTextSize("No Camera");
                ImGui::GetWindowDrawList()->AddText(
                    {
                        viewportPosition.x
                            + (availableSize.x - textSize.x) * 0.5F,
                        viewportPosition.y
                            + (availableSize.y - textSize.y) * 0.5F,
                    },
                    ImGui::GetColorU32(ImGuiCol_TextDisabled),
                    "No Camera"
                );
            }
        }
    } else {
        finishGizmoRecording();
        m_gizmoUsing = false;
    }

    m_selectedEntity = selectedEntity;

    ImGui::End();
}

bool Viewport::wantsCursorCapture() const noexcept {
    return m_editing && m_editorCamera.isLooking();
}

bool Viewport::drawSceneIcons(
    vshade::scene::Entity& selectedEntity,
    const float x,
    const float y,
    const float width,
    const float height,
    const bool allowInteraction
) {
    if (!m_scene || width <= 0.0F || height <= 0.0F) {
        return false;
    }

    ImDrawList& drawList = *ImGui::GetWindowDrawList();
    const vshade::math::Mat4 viewProjection =
        m_editorCamera.camera().viewProjection();
    const ImVec2 mouse = ImGui::GetMousePos();
    const float iconScale = ui::scaled(1.0F);
    const float hitRadius = 13.0F * iconScale;
    const float iconSpacing = 25.0F * iconScale;
    const auto cameraTexture = EditorIcons::camera();
    const auto directionalLightTexture = EditorIcons::directionalLight();
    const auto pointLightTexture = EditorIcons::pointLight();
    const auto speakerTexture = EditorIcons::speaker();
    const ImU32 darkIconColor = ImGui::ColorConvertFloat4ToU32(
        ui::color(ui::ColorRole::Chrome)
    );
    const ImVec4 iconBacking = ui::color(
        ui::ColorRole::SceneIconBackground
    );
    bool clicked = false;

    const auto entities = m_scene->view<
        const vshade::scene::UUIDComponent,
        const vshade::scene::TransformComponent>();
    for (const auto [handle, uuid, transformComponent] : entities.each()) {
        (void)handle;
        vshade::scene::Entity entity = m_scene->findEntity(uuid.uuid);
        if (!entity) {
            continue;
        }
        const auto* camera = entity.tryGet<vshade::scene::CameraComponent>();
        const auto* light = entity.tryGet<vshade::scene::LightComponent>();
        const auto* audio = entity.tryGet<vshade::scene::AudioSourceComponent>();
        const bool drawCamera = camera && m_showCameraGizmos;
        const bool drawLight = light && m_showLightGizmos;
        const bool drawAudio = audio && m_showAudioGizmos;
        const int iconCount = (drawCamera ? 1 : 0)
            + (drawLight ? 1 : 0)
            + (drawAudio ? 1 : 0);
        if (iconCount == 0) {
            continue;
        }

        const auto projected = projectToViewport(
            transformComponent.transform.position(),
            viewProjection,
            x,
            y,
            width,
            height
        );
        if (!projected) {
            continue;
        }

        int iconIndex = 0;
        const auto drawIcon = [&] (
            const char* label,
            const ImVec4 backing,
            const auto& drawIconBody
        ) {
            const float offset = (
                static_cast<float>(iconIndex)
                - static_cast<float>(iconCount - 1) * 0.5F
            ) * iconSpacing;
            ++iconIndex;
            const ImVec2 center{projected->x + offset, projected->y};
            drawList.AddCircleFilled(
                center,
                12.0F * iconScale,
                ImGui::ColorConvertFloat4ToU32(backing),
                24
            );
            if (entity == selectedEntity) {
                drawList.AddCircle(
                    center,
                    13.0F * iconScale,
                    ImGui::ColorConvertFloat4ToU32(
                        ui::color(ui::ColorRole::Accent)
                    ),
                    24,
                    2.0F * iconScale
                );
            }
            drawIconBody(center);

            const float deltaX = mouse.x - center.x;
            const float deltaY = mouse.y - center.y;
            const bool hovered = allowInteraction
                && deltaX * deltaX + deltaY * deltaY
                    <= hitRadius * hitRadius;
            if (hovered) {
                const std::string_view name = entity.name();
                ImGui::SetTooltip(
                    "%.*s\n%s",
                    static_cast<int>(name.size()),
                    name.data(),
                    label
                );
                if (!clicked && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selectedEntity = entity;
                    clicked = true;
                }
            }
        };

        if (drawCamera) {
            drawIcon(
                "Camera",
                iconBacking,
                [&](const ImVec2 center) {
                    if (!drawTextureSceneIcon(
                            drawList,
                            cameraTexture,
                            center,
                            iconScale
                        )) {
                        drawCameraSceneIcon(
                            drawList,
                            center,
                            darkIconColor,
                            iconScale
                        );
                    }
                }
            );
        }
        if (drawLight) {
            float iconAlpha = 1.0F;
            if (!light->enabled) {
                iconAlpha = 0.4F;
            }
            if (std::holds_alternative<vshade::renderer::DirectionalLight>(
                    light->light
                )) {
                drawIcon(
                    "Directional Light",
                    iconBacking,
                    [&](const ImVec2 center) {
                        if (!drawTextureSceneIcon(
                                drawList,
                                directionalLightTexture,
                                center,
                                iconScale,
                                iconAlpha
                            )) {
                            drawDirectionalLightSceneIcon(
                                drawList,
                                center,
                                darkIconColor,
                                iconScale
                            );
                        }
                    }
                );
            } else {
                drawIcon(
                    "Point Light",
                    iconBacking,
                    [&](const ImVec2 center) {
                        if (!drawTextureSceneIcon(
                                drawList,
                                pointLightTexture,
                                center,
                                iconScale,
                                iconAlpha
                            )) {
                            drawPointLightSceneIcon(
                                drawList,
                                center,
                                darkIconColor,
                                iconScale
                            );
                        }
                    }
                );
            }
        }
        if (drawAudio) {
            drawIcon(
                "Audio Source",
                iconBacking,
                [&](const ImVec2 center) {
                    if (!drawTextureSceneIcon(
                            drawList,
                            speakerTexture,
                            center,
                            iconScale,
                            1.0F
                        )) {
                        drawSpeakerSceneIcon(
                            drawList,
                            center,
                            darkIconColor,
                            iconScale
                        );
                    }
                }
            );
        }
    }
    return clicked;
}

void Viewport::drawGizmo(
    vshade::scene::Entity selectedEntity,
    const float x,
    const float y,
    const float width,
    const float height
) {
    if (!m_editing
        || m_gizmoOperation == GizmoOperation::None
        || !m_scene
        || !m_scene->valid(selectedEntity)
        || !selectedEntity.has<vshade::scene::TransformComponent>()) {
        finishGizmoRecording();
        m_gizmoUsing = false;
        return;
    }

    auto& transform = selectedEntity.transform();
    const vshade::math::Transform preManipulate = transform;
    vshade::math::Mat4 modelMatrix = transform.matrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(x, y, width, height);
    const ImGuizmo::MODE gizmoMode =
        m_gizmoOrientation == GizmoOrientation::Local
            ? ImGuizmo::LOCAL
            : ImGuizmo::WORLD;
    const float snap = m_gizmoOperation == GizmoOperation::Rotate
        ? m_rotationSnap
        : (m_gizmoOperation == GizmoOperation::Scale
            ? m_scaleSnap
            : m_translationSnap);
    const float snapValues[3]{snap, snap, snap};
    const bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(m_editorCamera.viewMatrix()),
        glm::value_ptr(m_editorCamera.projectionMatrix()),
        toImGuizmoOperation(m_gizmoOperation),
        gizmoMode,
        glm::value_ptr(modelMatrix),
        nullptr,
        m_snapEnabled ? snapValues : nullptr
    );

    if (manipulated) {
        float translation[3]{};
        float rotationDegrees[3]{};
        float scale[3]{};
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(modelMatrix),
            translation,
            rotationDegrees,
            scale
        );

        constexpr float minimumScale = 0.001F;
        transform.setPosition({translation[0], translation[1], translation[2]});
        transform.setRotation(vshade::math::fromEuler(glm::radians(
            vshade::math::Vec3{
                rotationDegrees[0],
                rotationDegrees[1],
                rotationDegrees[2],
            }
        )));
        transform.setScale({
            std::max(scale[0], minimumScale),
            std::max(scale[1], minimumScale),
            std::max(scale[2], minimumScale),
        });
    }

    const bool usingNow = ImGuizmo::IsUsing();
    if (usingNow && !m_gizmoRecording) {
        const vshade::math::Transform postManipulate = transform;
        transform = preManipulate;
        if (m_editHooks.begin) {
            m_editHooks.begin();
        }
        transform = postManipulate;
        m_gizmoRecording = true;
    }
    if (!usingNow) {
        finishGizmoRecording();
    }
    m_gizmoUsing = usingNow;
}

void Viewport::renderScene() {
    m_framebuffer->bind();
    try {
        vshade::renderer::Renderer::setClearColor({0.08F, 0.09F, 0.11F, 1.0F});
        vshade::renderer::Renderer::clear();
        m_framebuffer->clearEntityId(-1);
        m_runtimeCameraActive = false;

        if (m_editing) {
            if (m_scene && m_sceneRenderer) {
                m_sceneRenderer->render(*m_scene, m_editorCamera.camera());
            }
        } else if (m_runtime != nullptr && m_runtime->isPlaying()) {
            m_runtimeCameraActive = m_runtime->render(
                m_framebuffer->width(),
                m_framebuffer->height()
            );
        }
    } catch (...) {
        vshade::renderer::Framebuffer::unbind();
        throw;
    }
    vshade::renderer::Framebuffer::unbind();
}

void Viewport::queueSelectedCameraFrustum(
    const vshade::scene::Entity selectedEntity,
    const float aspectRatio
) const {
    if (!m_scene
        || !selectedEntity.valid()
        || !selectedEntity.has<vshade::scene::CameraComponent>()) {
        return;
    }

    const auto& camera =
        selectedEntity.get<vshade::scene::CameraComponent>();
    if (camera.projection != vshade::scene::CameraProjection::Perspective
        || !std::isfinite(camera.verticalFieldOfViewRadians)
        || !std::isfinite(aspectRatio)
        || !std::isfinite(camera.nearPlane)
        || !std::isfinite(camera.farPlane)
        || camera.verticalFieldOfViewRadians <= 0.0F
        || camera.verticalFieldOfViewRadians >= glm::pi<float>()
        || aspectRatio <= 0.0F
        || camera.nearPlane <= 0.0F
        || camera.farPlane <= camera.nearPlane) {
        return;
    }

    const auto& transform = selectedEntity.transform();
    const vshade::math::Vec3 position = transform.position();
    const vshade::math::Vec3 forward = transform.forward();
    const vshade::math::Vec3 right = transform.right();
    const vshade::math::Vec3 up = transform.up();

    const float halfFov = camera.verticalFieldOfViewRadians * 0.5F;
    const float nearHalfHeight = std::tan(halfFov) * camera.nearPlane;
    const float nearHalfWidth = nearHalfHeight * aspectRatio;
    const float farHalfHeight = std::tan(halfFov) * camera.farPlane;
    const float farHalfWidth = farHalfHeight * aspectRatio;
    const vshade::math::Vec3 nearCenter =
        position + forward * camera.nearPlane;
    const vshade::math::Vec3 farCenter =
        position + forward * camera.farPlane;

    const std::array<vshade::math::Vec3, 4> nearCorners{
        nearCenter + up * nearHalfHeight - right * nearHalfWidth,
        nearCenter + up * nearHalfHeight + right * nearHalfWidth,
        nearCenter - up * nearHalfHeight + right * nearHalfWidth,
        nearCenter - up * nearHalfHeight - right * nearHalfWidth,
    };
    const std::array<vshade::math::Vec3, 4> farCorners{
        farCenter + up * farHalfHeight - right * farHalfWidth,
        farCenter + up * farHalfHeight + right * farHalfWidth,
        farCenter - up * farHalfHeight + right * farHalfWidth,
        farCenter - up * farHalfHeight - right * farHalfWidth,
    };

    const ImVec4 accent = ui::color(ui::ColorRole::Accent);
    const vshade::math::Vec4 color{
        accent.x,
        accent.y,
        accent.z,
        accent.w,
    };
    for (std::size_t corner = 0; corner < nearCorners.size(); ++corner) {
        const std::size_t next = (corner + 1) % nearCorners.size();
        vshade::renderer::DebugDraw::line(
            nearCorners[corner], nearCorners[next], color
        );
        vshade::renderer::DebugDraw::line(
            farCorners[corner], farCorners[next], color
        );
        vshade::renderer::DebugDraw::line(
            nearCorners[corner], farCorners[corner], color
        );
    }
}

void Viewport::queueSelectedColliderGizmo(
    const vshade::scene::Entity selectedEntity
) const {
    if (!m_scene || !selectedEntity.valid()) {
        return;
    }

    const ImVec4 accent = ui::color(ui::ColorRole::Accent);
    const vshade::math::Vec4 color{
        accent.x, accent.y, accent.z, 0.9F
    };
    const auto& transform = selectedEntity.transform();
    const auto worldPoint = [&](const vshade::math::Vec3& point) {
        return transform.transformPoint(point);
    };
    const auto line = [&](
        const vshade::math::Vec3& first,
        const vshade::math::Vec3& second
    ) {
        vshade::renderer::DebugDraw::line(
            worldPoint(first),
            worldPoint(second),
            color
        );
    };
    const auto drawBox = [&](
        const vshade::math::Vec3& offset,
        const vshade::math::Vec3& halfExtents
    ) {
        const std::array<vshade::math::Vec3, 8> corners{
            offset + vshade::math::Vec3{-halfExtents.x, -halfExtents.y, -halfExtents.z},
            offset + vshade::math::Vec3{ halfExtents.x, -halfExtents.y, -halfExtents.z},
            offset + vshade::math::Vec3{ halfExtents.x,  halfExtents.y, -halfExtents.z},
            offset + vshade::math::Vec3{-halfExtents.x,  halfExtents.y, -halfExtents.z},
            offset + vshade::math::Vec3{-halfExtents.x, -halfExtents.y,  halfExtents.z},
            offset + vshade::math::Vec3{ halfExtents.x, -halfExtents.y,  halfExtents.z},
            offset + vshade::math::Vec3{ halfExtents.x,  halfExtents.y,  halfExtents.z},
            offset + vshade::math::Vec3{-halfExtents.x,  halfExtents.y,  halfExtents.z},
        };
        constexpr std::array<std::array<std::size_t, 2>, 12> edges{{
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7},
        }};
        for (const auto& edge : edges) {
            line(corners[edge[0]], corners[edge[1]]);
        }
    };
    const auto drawRing = [&](
        const vshade::math::Vec3& offset,
        const float firstRadius,
        const float secondRadius,
        const int firstAxis,
        const int secondAxis
    ) {
        constexpr std::size_t segments = 32;
        for (std::size_t segment = 0; segment < segments; ++segment) {
            const float firstAngle = glm::two_pi<float>()
                * static_cast<float>(segment) / static_cast<float>(segments);
            const float secondAngle = glm::two_pi<float>()
                * static_cast<float>(segment + 1) / static_cast<float>(segments);
            vshade::math::Vec3 first = offset;
            vshade::math::Vec3 second = offset;
            first[firstAxis] += std::cos(firstAngle) * firstRadius;
            first[secondAxis] += std::sin(firstAngle) * secondRadius;
            second[firstAxis] += std::cos(secondAngle) * firstRadius;
            second[secondAxis] += std::sin(secondAngle) * secondRadius;
            line(first, second);
        }
    };
    const auto drawSphere = [&](
        const vshade::math::Vec3& offset,
        const float radius
    ) {
        drawRing(offset, radius, radius, 0, 1);
        drawRing(offset, radius, radius, 0, 2);
        drawRing(offset, radius, radius, 1, 2);
    };
    const auto drawCylinder = [&](
        const vshade::math::Vec3& offset,
        const float halfHeight,
        const float radius,
        const bool capsule
    ) {
        drawRing(offset + vshade::math::Vec3{0.0F, halfHeight, 0.0F}, radius, radius, 0, 2);
        drawRing(offset - vshade::math::Vec3{0.0F, halfHeight, 0.0F}, radius, radius, 0, 2);
        constexpr std::array<vshade::math::Vec3, 4> directions{
            vshade::math::Vec3{1.0F, 0.0F, 0.0F},
            vshade::math::Vec3{-1.0F, 0.0F, 0.0F},
            vshade::math::Vec3{0.0F, 0.0F, 1.0F},
            vshade::math::Vec3{0.0F, 0.0F, -1.0F},
        };
        for (const auto& direction : directions) {
            line(
                offset + direction * radius + vshade::math::Vec3{0.0F, -halfHeight, 0.0F},
                offset + direction * radius + vshade::math::Vec3{0.0F, halfHeight, 0.0F}
            );
        }
        if (capsule) {
            drawRing(offset, radius, halfHeight + radius, 0, 1);
            drawRing(offset, radius, halfHeight + radius, 2, 1);
        }
    };
    const auto drawModelWireframe = [&](
        const vshade::asset::AssetReference<vshade::renderer::Model>& reference,
        const vshade::math::Vec3& offset
    ) {
        if (!m_assets || !reference.valid()) {
            return;
        }
        try {
            const auto model = m_assets->loadResource(reference).shared();
            const auto& vertices = model->collisionVertices();
            const auto& indices = model->collisionIndices();
            for (std::size_t index = 0; index + 2 < indices.size(); index += 3) {
                const vshade::math::Vec3 a = vertices[indices[index]] + offset;
                const vshade::math::Vec3 b = vertices[indices[index + 1]] + offset;
                const vshade::math::Vec3 c = vertices[indices[index + 2]] + offset;
                line(a, b);
                line(b, c);
                line(c, a);
            }
        } catch (const std::exception&) {
            // The Inspector reports asset errors; viewport gizmos stay non-fatal.
        }
    };

    if (const auto* box =
            selectedEntity.tryGet<vshade::scene::BoxCollider3DComponent>()) {
        drawBox(box->offset, box->halfExtents);
    } else if (const auto* sphere =
            selectedEntity.tryGet<vshade::scene::SphereCollider3DComponent>()) {
        drawSphere(sphere->offset, sphere->radius);
    } else if (const auto* capsule =
            selectedEntity.tryGet<vshade::scene::CapsuleCollider3DComponent>()) {
        drawCylinder(capsule->offset, capsule->halfHeight, capsule->radius, true);
    } else if (const auto* cylinder =
            selectedEntity.tryGet<vshade::scene::CylinderCollider3DComponent>()) {
        drawCylinder(cylinder->offset, cylinder->halfHeight, cylinder->radius, false);
    } else if (const auto* mesh =
            selectedEntity.tryGet<vshade::scene::MeshCollider3DComponent>()) {
        drawModelWireframe(mesh->model, mesh->offset);
    } else if (const auto* convex =
            selectedEntity.tryGet<vshade::scene::ConvexCollider3DComponent>()) {
        drawModelWireframe(convex->model, convex->offset);
    } else if (const auto* legacy =
            selectedEntity.tryGet<vshade::scene::Collider3DComponent>()) {
        std::visit(
            [&](const auto& shape) {
                using Shape = std::remove_cvref_t<decltype(shape)>;
                if constexpr (std::same_as<Shape, vshade::physics::BoxShape3D>) {
                    drawBox(legacy->offset, shape.halfExtents);
                } else if constexpr (
                    std::same_as<Shape, vshade::physics::SphereShape3D>
                ) {
                    drawSphere(legacy->offset, shape.radius);
                } else if constexpr (
                    std::same_as<Shape, vshade::physics::CapsuleShape3D>
                ) {
                    drawCylinder(legacy->offset, shape.halfHeight, shape.radius, true);
                } else if constexpr (
                    std::same_as<Shape, vshade::physics::CylinderShape3D>
                ) {
                    drawCylinder(legacy->offset, shape.halfHeight, shape.radius, false);
                }
            },
            legacy->shape
        );
    }
}

void Viewport::queuePhysicsGizmos() const {
    if (!m_scene) {
        return;
    }
    const ImVec4 warning = ui::color(ui::ColorRole::Warning);
    const vshade::math::Vec4 color{
        warning.x, warning.y, warning.z, 0.85F
    };
    for (const auto [handle, transform, rigidBody] : m_scene->view<
             const vshade::scene::TransformComponent,
             const vshade::scene::RigidBody3DComponent>().each()) {
        (void)handle;
        const vshade::math::Vec3 center = transform.transform.position();
        constexpr float markerSize = 0.12F;
        vshade::renderer::DebugDraw::line(
            center - vshade::math::Vec3{markerSize, 0.0F, 0.0F},
            center + vshade::math::Vec3{markerSize, 0.0F, 0.0F},
            color
        );
        vshade::renderer::DebugDraw::line(
            center - vshade::math::Vec3{0.0F, markerSize, 0.0F},
            center + vshade::math::Vec3{0.0F, markerSize, 0.0F},
            color
        );
        vshade::renderer::DebugDraw::line(
            center - vshade::math::Vec3{0.0F, 0.0F, markerSize},
            center + vshade::math::Vec3{0.0F, 0.0F, markerSize},
            color
        );
        if (glm::dot(
                rigidBody.settings.linearVelocity,
                rigidBody.settings.linearVelocity
            ) > 0.000001F) {
            vshade::renderer::DebugDraw::line(
                center,
                center + rigidBody.settings.linearVelocity,
                color
            );
        }
    }
}

void Viewport::selectEntityUnderMouse(
    vshade::scene::Entity& selectedEntity,
    const float x,
    const float y,
    const float width,
    const float height
) {
    if (!m_scene) {
        selectedEntity = {};
        return;
    }

    const auto cursor = cursorInViewport(
        x,
        y,
        width,
        height,
        m_framebuffer->width(),
        m_framebuffer->height()
    );
    if (!cursor) {
        return;
    }

    const std::int32_t entityId =
        m_framebuffer->readEntityId(cursor->pixelX, cursor->pixelY);
    selectedEntity = entityId == -1
        ? vshade::scene::Entity{}
        : m_scene->findEntityById(static_cast<std::uint32_t>(entityId));
}

vshade::math::Vec3 Viewport::spawnPositionAtCursor(
    const float x,
    const float y,
    const float width,
    const float height
) const {
    constexpr float fallbackDistance = 5.0F;
    const vshade::math::Ray fallbackRay = m_editorCamera.camera().worldRay(0.0F, 0.0F);
    const auto fallbackPosition = [&fallbackRay]() {
        if (vshade::math::lengthSquared(fallbackRay.direction) <= 0.0F) {
            return fallbackRay.origin;
        }
        return fallbackRay.origin + fallbackRay.direction * fallbackDistance;
    };

    const auto cursor = cursorInViewport(
        x,
        y,
        width,
        height,
        m_framebuffer->width(),
        m_framebuffer->height()
    );
    if (!cursor) {
        return fallbackPosition();
    }

    const vshade::math::Ray ray = m_editorCamera.camera().worldRay(
        cursor->ndcX,
        cursor->ndcY
    );
    const auto alongRay = [&ray]() {
        if (vshade::math::lengthSquared(ray.direction) <= 0.0F) {
            return ray.origin;
        }
        return ray.origin + ray.direction * fallbackDistance;
    };

    const std::int32_t entityId =
        m_framebuffer->readEntityId(cursor->pixelX, cursor->pixelY);
    if (entityId != -1) {
        const float depth = m_framebuffer->readDepth(cursor->pixelX, cursor->pixelY);
        if (std::isfinite(depth) && depth < 0.999F) {
            return m_editorCamera.camera().unproject({
                cursor->ndcX,
                cursor->ndcY,
                depth * 2.0F - 1.0F,
            });
        }
    }

    if (const auto groundHit = vshade::math::intersectPlane(
            ray,
            {0.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F}
        )) {
        return *groundHit;
    }
    return alongRay();
}

bool Viewport::spawnDroppedAsset(
    vshade::scene::Entity& selectedEntity,
    const float x,
    const float y,
    const float width,
    const float height
) {
    if (!m_editing || !m_scene || m_assets == nullptr) {
        return false;
    }

    const auto droppedPath = AssetSelector::acceptDroppedPath();
    if (!droppedPath) {
        return false;
    }

    const vshade::asset::AssetType type =
        vshade::asset::assetTypeFromExtension(droppedPath->extension());
    const vshade::math::Vec3 position = spawnPositionAtCursor(x, y, width, height);

    try {
        switch (type) {
            case vshade::asset::AssetType::Model: {
                const auto model = m_assets->reference<vshade::renderer::Model>(
                    *droppedPath
                );
                m_assets->load(model.handle());
                if (m_editHooks.begin) {
                    m_editHooks.begin();
                }
                vshade::scene::Entity entity = m_scene->create(
                    assetStemName(*droppedPath, "Model")
                );
                entity.transform().setPosition(position);
                entity.add<vshade::scene::ModelRendererComponent>(
                    vshade::scene::ModelRendererComponent{.model = model}
                );
                selectedEntity = entity;
                if (m_editHooks.commit) {
                    m_editHooks.commit();
                }
                return true;
            }
            case vshade::asset::AssetType::Prefab: {
                const auto prefab = m_assets->loadResource<vshade::scene::Prefab>(
                    *droppedPath
                );
                if (m_editHooks.begin) {
                    m_editHooks.begin();
                }
                vshade::scene::Entity entity = m_scene->instantiate(
                    *prefab,
                    prefab.reference()
                );
                if (!entity) {
                    throw std::runtime_error("The prefab did not contain a root entity");
                }
                entity.transform().setPosition(position);
                selectedEntity = entity;
                if (m_editHooks.commit) {
                    m_editHooks.commit();
                }
                return true;
            }
            default:
                ENGINE_WARN(
                    "Viewport drop currently supports models and prefabs: '{}'",
                    droppedPath->generic_string()
                );
                return false;
        }
    } catch (const std::exception& error) {
        if (m_editHooks.revert) {
            m_editHooks.revert();
        }
        ENGINE_ERROR(
            "Failed to spawn asset '{}': {}",
            droppedPath->generic_string(),
            error.what()
        );
        return false;
    }
}

void Viewport::resizeFramebuffer(const float width, const float height) {
    constexpr float maximumDimension =
        static_cast<float>(std::numeric_limits<std::uint32_t>::max());
    if (!std::isfinite(width) || !std::isfinite(height)
        || width < 1.0F || height < 1.0F
        || width > maximumDimension || height > maximumDimension) {
        return;
    }

    const auto framebufferWidth = static_cast<std::uint32_t>(width);
    const auto framebufferHeight = static_cast<std::uint32_t>(height);
    if (framebufferWidth != m_framebuffer->width()
        || framebufferHeight != m_framebuffer->height()) {
        m_framebuffer->resize(framebufferWidth, framebufferHeight);
    }
}

} // namespace editor
