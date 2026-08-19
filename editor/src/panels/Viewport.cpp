#include "panels/Viewport.hpp"

#include <input/Input.hpp>
#include <input/KeyCode.hpp>
#include <input/MouseCode.hpp>
#include <math/Quaternion.hpp>
#include <renderer/DebugDraw.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>
#include <scene/Scene.hpp>
#include <scene/components/CoreComponents.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

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

void drawWireCube(const vshade::math::Transform& transform) {
    constexpr std::array<vshade::math::Vec3, 8> corners{{
        {-0.5F, -0.5F, -0.5F},
        { 0.5F, -0.5F, -0.5F},
        { 0.5F,  0.5F, -0.5F},
        {-0.5F,  0.5F, -0.5F},
        {-0.5F, -0.5F,  0.5F},
        { 0.5F, -0.5F,  0.5F},
        { 0.5F,  0.5F,  0.5F},
        {-0.5F,  0.5F,  0.5F},
    }};
    constexpr std::array<std::pair<std::size_t, std::size_t>, 12> edges{{
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    }};

    for (const auto [start, end] : edges) {
        vshade::renderer::DebugDraw::line(
            transform.transformPoint(corners[start]),
            transform.transformPoint(corners[end]),
            {0.2F, 0.75F, 1.0F, 1.0F}
        );
    }
}

} // namespace

Viewport::Viewport()
    : m_framebuffer(
          std::make_unique<vshade::renderer::Framebuffer>(1280, 720)
      ),
      m_editorCamera(0.785398163F, 16.0F / 9.0F, 0.1F, 1000.0F) {}

Viewport::~Viewport() = default;

void Viewport::onUpdate(const float deltaTime) {
    using vshade::input::Input;
    using vshade::input::KeyCode;
    using vshade::input::MouseButton;

    const bool cameraLookActive =
        Input::isMouseButtonDown(MouseButton::Right);
    if (m_hovered && !m_gizmoUsing && !cameraLookActive) {
        if (Input::isKeyPressed(KeyCode::W)) {
            m_gizmoOperation = GizmoOperation::Translate;
        } else if (Input::isKeyPressed(KeyCode::E)) {
            m_gizmoOperation = GizmoOperation::Rotate;
        } else if (Input::isKeyPressed(KeyCode::R)) {
            m_gizmoOperation = GizmoOperation::Scale;
        }
    }

    m_editorCamera.setInputEnabled(m_hovered && !m_gizmoUsing);
    m_editorCamera.onUpdate(deltaTime);
}

void Viewport::setScene(
    std::shared_ptr<vshade::scene::Scene> scene
) {
    m_scene = std::move(scene);
    m_selectedEntity = {};
}

void Viewport::setSelectedEntity(
    const vshade::scene::Entity entity
) noexcept {
    m_selectedEntity = entity;
}

void Viewport::onImGuiRender() {
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
        drawGizmo(
            viewportPosition.x,
            viewportPosition.y,
            availableSize.x,
            availableSize.y
        );
    } else {
        m_gizmoUsing = false;
    }

    ImGui::End();
}

bool Viewport::wantsCursorCapture() const noexcept {
    return m_editorCamera.isLooking();
}

void Viewport::drawGizmo(
    const float x,
    const float y,
    const float width,
    const float height
) {
    if (m_gizmoOperation == GizmoOperation::None
        || !m_scene
        || !m_scene->valid(m_selectedEntity)
        || !m_selectedEntity.has<vshade::scene::TransformComponent>()) {
        m_gizmoUsing = false;
        return;
    }

    auto& transform = m_selectedEntity.transform();
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

        if (m_scene) {
            const auto entities = std::as_const(*m_scene).view<
                vshade::scene::TransformComponent
            >();
            for (const auto handle : entities) {
                const auto& component =
                    entities.get<vshade::scene::TransformComponent>(handle);
                drawWireCube(component.transform);
            }
        }
        vshade::renderer::DebugDraw::line(
            {0.0F, 0.0F, 0.0F},
            {2.0F, 0.0F, 0.0F},
            {1.0F, 0.2F, 0.2F, 1.0F}
        );
        vshade::renderer::DebugDraw::line(
            {0.0F, 0.0F, 0.0F},
            {0.0F, 2.0F, 0.0F},
            {0.2F, 1.0F, 0.2F, 1.0F}
        );
        vshade::renderer::DebugDraw::line(
            {0.0F, 0.0F, 0.0F},
            {0.0F, 0.0F, 2.0F},
            {0.2F, 0.4F, 1.0F, 1.0F}
        );
        vshade::renderer::DebugDraw::flush(m_editorCamera.camera());
    } catch (...) {
        vshade::renderer::DebugDraw::clear();
        vshade::renderer::Framebuffer::unbind();
        throw;
    }
    vshade::renderer::Framebuffer::unbind();
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
