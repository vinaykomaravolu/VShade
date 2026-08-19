#include "panels/Viewport.hpp"

#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <input/MouseCode.hpp>
#include <math/Quaternion.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneRenderer.hpp>
#include <scene/SceneRuntime.hpp>
#include <scene/components/CoreComponents.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

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

} // namespace

Viewport::Viewport(vshade::asset::AssetManager& assets)
    : m_framebuffer(
          std::make_unique<vshade::renderer::Framebuffer>(1280, 720)
      ),
      m_sceneRenderer(
          std::make_unique<vshade::scene::SceneRenderer>(assets)
      ),
      m_editorCamera(0.785398163F, 16.0F / 9.0F, 0.1F, 1000.0F) {}

Viewport::~Viewport() = default;

void Viewport::onUpdate(const float deltaTime) {
    using vshade::input::Input;
    using vshade::input::KeyCode;
    using vshade::input::MouseButton;

    const bool cameraLookActive =
        Input::isMouseButtonDown(MouseButton::Right);
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
        m_editing && m_visible && m_hovered && !m_gizmoUsing
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
        m_editorCamera.setInputEnabled(false);
    }
}

void Viewport::setVisible(const bool visible) noexcept {
    m_visible = visible;
    if (!visible) {
        m_hovered = false;
        m_gizmoUsing = false;
    }
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
            drawGizmo(
                selectedEntity,
                viewportPosition.x,
                viewportPosition.y,
                availableSize.x,
                availableSize.y
            );
            if (imageHovered
                && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
                && !ImGuizmo::IsOver()) {
                selectEntityUnderMouse(
                    selectedEntity,
                    viewportPosition.x,
                    viewportPosition.y,
                    availableSize.x,
                    availableSize.y
                );
            }
        } else {
            m_gizmoUsing = false;
            if (!m_runtimeCameraActive) {
                const ImVec2 textSize = ImGui::CalcTextSize("No Camera");
                ImGui::SetCursorScreenPos({
                    viewportPosition.x + (availableSize.x - textSize.x) * 0.5F,
                    viewportPosition.y + (availableSize.y - textSize.y) * 0.5F,
                });
                ImGui::TextUnformatted("No Camera");
            }
        }
    } else {
        m_gizmoUsing = false;
    }

    ImGui::End();
}

bool Viewport::wantsCursorCapture() const noexcept {
    return m_editing && m_editorCamera.isLooking();
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
        m_gizmoUsing = false;
        return;
    }

    auto& transform = selectedEntity.transform();
    vshade::math::Mat4 modelMatrix = transform.matrix();

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(x, y, width, height);
    const bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(m_editorCamera.viewMatrix()),
        glm::value_ptr(m_editorCamera.projectionMatrix()),
        toImGuizmoOperation(m_gizmoOperation),
        ImGuizmo::LOCAL,
        glm::value_ptr(modelMatrix)
    );
    m_gizmoUsing = ImGuizmo::IsUsing();

    if (!manipulated) {
        return;
    }

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

void Viewport::selectEntityUnderMouse(
    vshade::scene::Entity& selectedEntity,
    const float x,
    const float y,
    const float width,
    const float height
) {
    if (!m_scene || width <= 0.0F || height <= 0.0F) {
        selectedEntity = {};
        return;
    }

    const ImVec2 mouse = ImGui::GetMousePos();
    const float localX = mouse.x - x;
    const float localY = mouse.y - y;
    if (localX < 0.0F || localY < 0.0F
        || localX >= width || localY >= height) {
        return;
    }

    const std::uint32_t framebufferWidth = m_framebuffer->width();
    const std::uint32_t framebufferHeight = m_framebuffer->height();
    const auto pixelX = std::min(
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
    const std::uint32_t pixelY = framebufferHeight - 1 - topDownPixelY;
    const std::int32_t entityId = m_framebuffer->readEntityId(pixelX, pixelY);
    selectedEntity = entityId == -1
        ? vshade::scene::Entity{}
        : m_scene->findEntityById(static_cast<std::uint32_t>(entityId));
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
