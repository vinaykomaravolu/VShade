#include "panels/Viewport.hpp"

#include <renderer/DebugDraw.hpp>
#include <renderer/Framebuffer.hpp>
#include <renderer/Renderer.hpp>
#include <scene/Scene.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include <imgui.h>

namespace editor {
namespace {

constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoCollapse;

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
    m_editorCamera.setInputEnabled(m_hovered);
    m_editorCamera.onUpdate(deltaTime);
}

void Viewport::setScene(
    std::shared_ptr<vshade::scene::Scene> scene,
    const vshade::scene::Entity previewEntity
) {
    m_scene = std::move(scene);
    m_previewEntity = previewEntity;
}

void Viewport::onImGuiRender() {
    const bool visible = ImGui::Begin("Viewport", nullptr, panelFlags);
    m_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    if (visible) {
        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        resizeFramebuffer(availableSize.x, availableSize.y);
        m_editorCamera.setViewportSize(availableSize.x, availableSize.y);
        renderScene();

        const ImTextureID textureId =
            static_cast<ImTextureID>(m_framebuffer->colorAttachmentId());
        ImGui::Image(
            ImTextureRef{textureId},
            availableSize,
            {0.0F, 1.0F},
            {1.0F, 0.0F}
        );
    }

    ImGui::End();
}

bool Viewport::wantsCursorCapture() const noexcept {
    return m_editorCamera.isLooking();
}

void Viewport::renderScene() {
    m_framebuffer->bind();
    try {
        vshade::renderer::Renderer::setClearColor({0.08F, 0.09F, 0.11F, 1.0F});
        vshade::renderer::Renderer::clear();

        if (m_scene && m_scene->valid(m_previewEntity)) {
            drawWireCube(m_previewEntity.transform());
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
