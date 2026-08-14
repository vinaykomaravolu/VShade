#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <math/Quaternion.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneLightingSystem.hpp>

TEST_CASE("SceneLightingSystem collects environment and world-space entity lights", "[scene][lighting]") {
    vshade::scene::Scene scene("Lighting collection");
    scene.setEnvironment({
        .ambientColor = {0.2F, 0.3F, 0.5F},
        .ambientIntensity = 0.25F,
    });

    vshade::scene::Entity sun = scene.createEntity("Sun");
    auto& sunTransform = sun.component<vshade::scene::TransformComponent>().transform;
    sunTransform.setRotation(vshade::math::fromEuler({0.0F, 1.57079632679F, 0.0F}));
    sun.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::DirectionalLight{
            .direction = {0.0F, 0.0F, -1.0F},
            .intensity = 2.0F,
        },
        true
    );

    vshade::scene::Entity lamp = scene.createEntity("Lamp");
    auto& lampTransform = lamp.component<vshade::scene::TransformComponent>().transform;
    lampTransform.setPosition({2.0F, 3.0F, 4.0F});
    lampTransform.setScale({10.0F, 10.0F, 10.0F});
    lamp.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::PointLight{
            .position = {1.0F, 0.0F, 0.0F},
            .range = 8.0F,
        },
        true
    );

    vshade::scene::Entity disabled = scene.createEntity("Disabled lamp");
    disabled.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::PointLight{},
        false
    );

    const vshade::renderer::Lighting lighting =
        vshade::scene::SceneLightingSystem::collect(scene);

    REQUIRE(lighting.ambientLight().has_value());
    CHECK(lighting.ambientLight()->color.b == Catch::Approx(0.5F));
    CHECK(lighting.ambientLight()->intensity == Catch::Approx(0.25F));
    REQUIRE(lighting.directionalLights().size() == 1);
    CHECK(lighting.directionalLights()[0].direction.x ==
          Catch::Approx(sunTransform.forward().x).margin(0.0001F));
    CHECK(lighting.directionalLights()[0].direction.z ==
          Catch::Approx(sunTransform.forward().z).margin(0.0001F));
    REQUIRE(lighting.pointLights().size() == 1);
    CHECK(lighting.pointLights()[0].position.x == Catch::Approx(3.0F));
    CHECK(lighting.pointLights()[0].position.y == Catch::Approx(3.0F));
    CHECK(lighting.pointLights()[0].position.z == Catch::Approx(4.0F));
    CHECK(lighting.pointLights()[0].range == Catch::Approx(8.0F));
}
