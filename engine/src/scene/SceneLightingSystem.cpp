#include "scene/SceneLightingSystem.hpp"

#include "scene/Components.hpp"
#include "scene/Scene.hpp"

#include <type_traits>
#include <variant>

namespace vshade::scene {

renderer::Lighting SceneLightingSystem::collect(const Scene& scene) {
    renderer::Lighting lighting;
    lighting.clear();
    lighting.setAmbientLight({
        .color = scene.environment().ambientColor,
        .intensity = scene.environment().ambientIntensity,
    });

    const auto lights = scene.view<const TransformComponent, const LightComponent>();
    lights.each([&lighting](
        const TransformComponent& transform,
        const LightComponent& component
    ) {
        if (!component.enabled) {
            return;
        }
        std::visit(
            [&lighting, &transform](const auto& localLight) {
                using LightType = std::decay_t<decltype(localLight)>;
                if constexpr (std::is_same_v<LightType, renderer::DirectionalLight>) {
                    renderer::DirectionalLight worldLight = localLight;
                    worldLight.direction = transform.transform.transformDirection(
                        localLight.direction
                    );
                    lighting.addDirectionalLight(worldLight);
                } else {
                    renderer::PointLight worldLight = localLight;
                    worldLight.position = transform.transform.position() +
                        transform.transform.transformDirection(localLight.position);
                    lighting.addPointLight(worldLight);
                }
            },
            component.light
        );
    });
    return lighting;
}

} // namespace vshade::scene
