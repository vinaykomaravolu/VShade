#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <renderer/Lighting.hpp>

#include <stdexcept>
#include <type_traits>

static_assert(std::is_constructible_v<
    vshade::renderer::Light,
    vshade::renderer::DirectionalLight
>);
static_assert(std::is_constructible_v<
    vshade::renderer::Light,
    vshade::renderer::PointLight
>);
static_assert(!std::is_constructible_v<
    vshade::renderer::Light,
    vshade::renderer::AmbientLight
>);

TEST_CASE("Lighting stores one ambient and multiple local lights", "[lighting]") {
    vshade::renderer::Lighting lighting;

    REQUIRE(lighting.ambientLight().has_value());
    CHECK(lighting.ambientLight()->intensity == Catch::Approx(0.15F));
    CHECK(lighting.directionalLights().empty());
    CHECK(lighting.pointLights().empty());

    lighting.setAmbientLight({.color = {0.2F, 0.3F, 0.4F}, .intensity = 0.25F});
    lighting.addDirectionalLight({.direction = {0.0F, -2.0F, 0.0F}});
    lighting.addDirectionalLight({.direction = {1.0F, -1.0F, 0.0F}});
    lighting.addPointLight({.position = {-2.0F, 1.0F, 0.0F}, .range = 8.0F});
    lighting.addPointLight({.position = {2.0F, 1.0F, 0.0F}, .range = 12.0F});

    REQUIRE(lighting.ambientLight().has_value());
    CHECK(lighting.ambientLight()->color.b == Catch::Approx(0.4F));
    REQUIRE(lighting.directionalLights().size() == 2);
    CHECK(lighting.directionalLights()[0].direction.y == Catch::Approx(-1.0F));
    CHECK(lighting.pointLights().size() == 2);

    lighting.clearAmbientLight();
    CHECK_FALSE(lighting.ambientLight().has_value());
    lighting.clear();
    CHECK(lighting.directionalLights().empty());
    CHECK(lighting.pointLights().empty());
}

TEST_CASE("Lighting rejects invalid values and collection overflow", "[lighting]") {
    vshade::renderer::Lighting lighting;

    CHECK_THROWS_AS(
        lighting.setAmbientLight({.color = {-1.0F, 1.0F, 1.0F}}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        lighting.addDirectionalLight({.direction = {0.0F, 0.0F, 0.0F}}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        lighting.addPointLight({.range = 0.0F}),
        std::invalid_argument
    );

    for (std::size_t index = 0; index < vshade::renderer::maximumDirectionalLights; ++index) {
        lighting.addDirectionalLight({});
    }
    CHECK_THROWS_AS(lighting.addDirectionalLight({}), std::length_error);

    for (std::size_t index = 0; index < vshade::renderer::maximumPointLights; ++index) {
        lighting.addPointLight({});
    }
    CHECK_THROWS_AS(lighting.addPointLight({}), std::length_error);
}
