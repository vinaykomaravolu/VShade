#include <catch2/catch_test_macros.hpp>

#include <scene/Components.hpp>
#include <scene/Scene.hpp>

#include <string>

namespace {

struct HealthComponent {
    int points = 100;
};

} // namespace

TEST_CASE("Scene owns entities and supports custom components", "[scene]") {
    vshade::scene::Scene scene;
    vshade::scene::Entity player = scene.createEntity("Player");

    REQUIRE(player.valid());
    CHECK(player.hasComponents<
        vshade::scene::TagComponent,
        vshade::scene::TransformComponent
    >());
    CHECK(player.component<vshade::scene::TagComponent>().tag == "Player");

    player.addComponent<HealthComponent>(75);
    CHECK(player.component<HealthComponent>().points == 75);

    std::size_t entityCount = 0;
    for (const auto handle : scene.view<vshade::scene::TransformComponent>()) {
        (void)handle;
        ++entityCount;
    }
    CHECK(entityCount == 1);

    scene.destroyEntity(player);
    CHECK_FALSE(player.valid());
}
