#include "scene/SceneRenderer.hpp"

#include "asset/AssetManager.hpp"
#include "renderer/Camera.hpp"
#include "renderer/DebugDraw.hpp"
#include "renderer/Model.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Renderer2D.hpp"
#include "renderer/Renderer3D.hpp"
#include "renderer/Texture.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "scene/SceneLightingSystem.hpp"

#include <entt/entity/entity.hpp>

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace vshade::scene {

struct SceneRenderer::Impl {
    explicit Impl(asset::AssetManager& manager) : assets(manager) {}

    asset::AssetManager& assets;
    renderer::Camera camera;
    renderer::DebugFrameStats frameStats;
    bool hasCamera = false;
};

SceneRenderer::SceneRenderer(asset::AssetManager& assets)
    : m_impl(std::make_unique<Impl>(assets)) {}

SceneRenderer::~SceneRenderer() = default;
SceneRenderer::SceneRenderer(SceneRenderer&&) noexcept = default;
SceneRenderer& SceneRenderer::operator=(SceneRenderer&&) noexcept = default;

bool SceneRenderer::render(
    Scene& scene,
    const std::uint32_t viewportWidth,
    const std::uint32_t viewportHeight
) {
    if (viewportWidth == 0 || viewportHeight == 0) {
        m_impl->hasCamera = false;
        return false;
    }

    const TransformComponent* cameraTransform = nullptr;
    const CameraComponent* cameraSettings = nullptr;
    const auto cameras = scene.view<const TransformComponent, const CameraComponent>();
    for (const auto [handle, transform, camera] : cameras.each()) {
        (void)handle;
        if (camera.active &&
            (cameraSettings == nullptr || camera.priority > cameraSettings->priority)) {
            cameraTransform = &transform;
            cameraSettings = &camera;
        }
    }
    if (cameraSettings == nullptr || cameraTransform == nullptr) {
        m_impl->hasCamera = false;
        return false;
    }

    const float aspect = static_cast<float>(viewportWidth) /
        static_cast<float>(viewportHeight);
    if (cameraSettings->projection == CameraProjection::Perspective) {
        m_impl->camera.setPerspective(
            cameraSettings->verticalFieldOfViewRadians,
            aspect,
            cameraSettings->nearPlane,
            cameraSettings->farPlane
        );
    } else {
        const float halfHeight = cameraSettings->orthographicHeight * 0.5F;
        const float halfWidth = halfHeight * aspect;
        m_impl->camera.setOrthographic(
            -halfWidth,
            halfWidth,
            -halfHeight,
            halfHeight,
            cameraSettings->nearPlane,
            cameraSettings->farPlane
        );
    }
    const math::Transform& transform = cameraTransform->transform;
    m_impl->camera.lookAt(
        transform.position(),
        transform.position() + transform.forward(),
        transform.up()
    );
    m_impl->hasCamera = true;

    renderer::ClearFlags clearFlags = renderer::ClearFlags::None;
    if (cameraSettings->clearColorEnabled) {
        renderer::Renderer::setClearColor(cameraSettings->clearColor);
        clearFlags |= renderer::ClearFlags::Color;
    }
    if (cameraSettings->clearDepthEnabled) {
        clearFlags |= renderer::ClearFlags::Depth;
    }
    if (clearFlags != renderer::ClearFlags::None) {
        renderer::Renderer::clear(clearFlags);
    }

    return render(scene, m_impl->camera);
}

bool SceneRenderer::render(
    Scene& scene,
    const renderer::Camera& camera
) {
    m_impl->camera = camera;
    m_impl->hasCamera = true;

    renderer::Renderer3D::setLighting(SceneLightingSystem::collect(scene));
    renderer::Renderer3D::beginScene(camera);
    const auto models = scene.view<const TransformComponent, const ModelRendererComponent>();
    for (const auto [handle, modelTransform, modelRenderer] : models.each()) {
        (void)handle;
        if (!modelRenderer.visible || !modelRenderer.model.valid()) {
            continue;
        }
        const auto model = m_impl->assets.loadResource(modelRenderer.model);
        const auto entityId = static_cast<std::int32_t>(
            entt::to_integral(handle)
        );
        renderer::Renderer3D::drawModel(
            modelTransform.transform,
            *model,
            {},
            entityId
        );
    }
    renderer::Renderer3D::endScene();

    renderer::Renderer2D::beginScene(camera);
    const auto sprites = scene.view<const TransformComponent, const SpriteRendererComponent>();
    for (const auto [handle, spriteTransform, sprite] : sprites.each()) {
        const auto entityId = static_cast<std::int32_t>(
            entt::to_integral(handle)
        );
        if (sprite.texture.valid()) {
            const auto texture = m_impl->assets.loadResource(sprite.texture);
            renderer::Renderer2D::drawQuad(
                spriteTransform.transform,
                *texture,
                sprite.color,
                sprite.tiling,
                sprite.sortingLayer,
                entityId
            );
        } else if (!sprite.texturePath.empty()) {
            const auto texture = m_impl->assets.loadResource<renderer::Texture2D>(
                sprite.texturePath
            );
            renderer::Renderer2D::drawQuad(
                spriteTransform.transform,
                *texture,
                sprite.color,
                sprite.tiling,
                sprite.sortingLayer,
                entityId
            );
        } else {
            renderer::Renderer2D::drawQuad(
                spriteTransform.transform,
                sprite.color,
                sprite.sortingLayer,
                entityId
            );
        }
    }
    renderer::Renderer2D::endScene();

    const std::size_t debugLines = renderer::DebugDraw::lineCount();
    if (debugLines != 0) {
        renderer::DebugDraw::flush(camera);
    }
    const auto& modelStats = renderer::Renderer3D::stats();
    const auto& spriteStats = renderer::Renderer2D::stats();
    std::uint64_t entityCount = 0;
    for (const auto handle : scene.view<const TransformComponent>()) {
        (void)handle;
        ++entityCount;
    }
    m_impl->frameStats = {
        .rendering = renderer::Renderer::stats(),
        .entityCount = entityCount,
        .modelDrawCalls = modelStats.drawCalls,
        .meshCount = modelStats.meshCount,
        .spriteDrawCalls = spriteStats.drawCalls,
        .quadCount = spriteStats.quadCount,
        .debugLineCount = debugLines,
    };
    return true;
}

const renderer::DebugFrameStats& SceneRenderer::frameStats() const noexcept {
    static const renderer::DebugFrameStats empty;
    return m_impl ? m_impl->frameStats : empty;
}

const renderer::Camera* SceneRenderer::activeCamera() const noexcept {
    return m_impl && m_impl->hasCamera ? &m_impl->camera : nullptr;
}

} // namespace vshade::scene
