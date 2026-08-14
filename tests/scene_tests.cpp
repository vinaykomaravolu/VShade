#include <catch2/catch_test_macros.hpp>

#include <core/filesystem.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneComponentRegistry.hpp>
#include <scene/SceneSerializer.hpp>

#include <catch2/catch_approx.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

namespace {

const std::filesystem::path playerTexturePath{"assets/player.png"};

[[nodiscard]] std::filesystem::path sceneOutputPath(const char* filename) {
    const std::filesystem::path directory{VSHADE_SCENE_OUTPUT_DIR};
    std::filesystem::create_directories(directory);
    return directory / filename;
}

struct HealthComponent {
    int points = 100;
};

struct WeaponComponent {
    std::string definitionPath;
    int ammunition = 0;
};

struct PositionComponent {
    float x = 0.0F;
};

struct VelocityComponent {
    float x = 0.0F;
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

TEST_CASE("Scene views support EnTT iteration styles", "[scene]") {
    vshade::scene::Scene scene("View iteration");
    vshade::scene::Entity first = scene.createEntity("First");
    first.addComponent<PositionComponent>(1.0F);
    first.addComponent<VelocityComponent>(2.0F);

    vshade::scene::Entity second = scene.createEntity("Second");
    second.addComponent<PositionComponent>(3.0F);
    second.addComponent<VelocityComponent>(4.0F);

    auto view = scene.view<const PositionComponent, VelocityComponent>();

    // A component-only callback omits the entity identifier.
    view.each([](const PositionComponent& position, VelocityComponent& velocity) {
        velocity.x += position.x;
    });

    // An extended callback receives the entity before its components.
    std::size_t extendedCallbackCount = 0;
    view.each([&extendedCallbackCount](
        const entt::entity entity,
        const PositionComponent& position,
        VelocityComponent& velocity
    ) {
        CHECK(entity != entt::null);
        velocity.x += position.x;
        ++extendedCallbackCount;
    });
    CHECK(extendedCallbackCount == 2);

    // each() also exposes tuples for structured range iteration.
    std::size_t rangeCount = 0;
    for (auto [entity, position, velocity] : view.each()) {
        CHECK(entity != entt::null);
        CHECK(position.x > 0.0F);
        CHECK(velocity.x > position.x);
        ++rangeCount;
    }
    CHECK(rangeCount == 2);

    // Iterating entity identifiers allows requesting only components of interest.
    float totalVelocity = 0.0F;
    for (const entt::entity entity : view) {
        VelocityComponent& velocity = view.get<VelocityComponent>(entity);
        totalVelocity += velocity.x;
    }
    CHECK(totalVelocity == Catch::Approx(14.0F));
}

TEST_CASE("Scene duplicates built-in components with a new UUID", "[scene]") {
    REQUIRE(std::filesystem::is_regular_file(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "player.png"
    ));
    vshade::scene::Scene scene("Duplication");
    vshade::scene::Entity original = scene.createEntity("Player");
    original.component<vshade::scene::TransformComponent>().transform.setPosition(
        {1.0F, 2.0F, 3.0F}
    );
    original.addComponent<vshade::scene::SpriteRendererComponent>(
        playerTexturePath,
        vshade::math::Vec4{0.8F, 0.9F, 1.0F, 1.0F},
        vshade::math::Vec2{2.0F, 1.0F},
        4
    );

    const vshade::scene::Entity duplicate = scene.duplicateEntity(original);
    REQUIRE(duplicate.valid());
    CHECK(duplicate.uuid() != original.uuid());
    CHECK(duplicate.component<vshade::scene::TagComponent>().tag == "Player Copy");
    CHECK(duplicate.component<vshade::scene::TransformComponent>().transform.position().x ==
          Catch::Approx(1.0F));
    CHECK(duplicate.component<vshade::scene::SpriteRendererComponent>().texturePath ==
          playerTexturePath);
}

TEST_CASE("Scene serialization round trips stable components", "[scene]") {
    const std::filesystem::path path = sceneOutputPath("roundtrip.json");

    vshade::scene::Scene source("Example");
    vshade::scene::Entity player = source.createEntity("Player");
    const std::uint64_t playerUuid = player.uuid();
    auto& transform = player.component<vshade::scene::TransformComponent>().transform;
    transform.setPosition({0.0F, 1.0F, 0.0F});
    transform.setRotation(vshade::math::fromEuler({0.1F, 0.2F, 0.3F}));
    transform.setScale({1.0F, 2.0F, 1.0F});
    player.addComponent<vshade::scene::SpriteRendererComponent>(
        playerTexturePath,
        vshade::math::Vec4{1.0F, 0.8F, 0.6F, 1.0F},
        vshade::math::Vec2{1.0F, 2.0F},
        7
    );

    vshade::scene::SceneSerializer writer(source);
    REQUIRE(writer.serialize(path));

    vshade::scene::Scene loaded("Old name");
    const vshade::scene::Entity replaced =
        loaded.createEntity("This entity should be replaced");
    (void)replaced;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));
    CHECK_FALSE(replaced.valid());

    CHECK(loaded.name() == "Example");
    const vshade::scene::Entity loadedPlayer = loaded.findEntity(playerUuid);
    REQUIRE(loadedPlayer.valid());
    CHECK(loadedPlayer.component<vshade::scene::TagComponent>().tag == "Player");
    const auto& loadedTransform =
        loadedPlayer.component<vshade::scene::TransformComponent>().transform;
    CHECK(loadedTransform.position().y == Catch::Approx(1.0F));
    CHECK(loadedTransform.scale().y == Catch::Approx(2.0F));
    CHECK(loadedTransform.rotation().w == Catch::Approx(transform.rotation().w));
    CHECK(loadedTransform.rotation().x == Catch::Approx(transform.rotation().x));
    const auto& loadedSprite =
        loadedPlayer.component<vshade::scene::SpriteRendererComponent>();
    CHECK(loadedSprite.texturePath == playerTexturePath);
    CHECK(loadedSprite.color.g == Catch::Approx(0.8F));
    CHECK(loadedSprite.tiling.y == Catch::Approx(2.0F));
    CHECK(loadedSprite.sortingLayer == 7);

}

TEST_CASE("Scene loading is transactional for malformed files", "[scene]") {
    const std::filesystem::path path = sceneOutputPath("malformed.json");
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output << R"json({"FormatVersion":1,"Scene":"Broken","Entities":[})json";
    }

    vshade::scene::Scene scene("Keep me");
    const vshade::scene::Entity survivor = scene.createEntity("Survivor");
    const std::uint64_t survivorUuid = survivor.uuid();
    vshade::scene::SceneSerializer serializer(scene);

    CHECK_FALSE(serializer.deserialize(path));
    CHECK_FALSE(serializer.lastError().empty());
    CHECK(scene.name() == "Keep me");
    CHECK(scene.findEntity(survivorUuid).valid());

}

TEST_CASE("Custom components round trip through a scene file", "[scene]") {
    using vshade::scene::SceneComponentRegistry;

    // Register each custom component type once during application startup.
    SceneComponentRegistry::registerComponent<HealthComponent>();
    SceneComponentRegistry::registerComponent<WeaponComponent>("Weapon");

    // Create a scene containing multiple entities and components.
    vshade::scene::Scene source("Simple scene");

    vshade::scene::Entity player = source.createEntity("Player");
    player.addComponent<HealthComponent>(100);
    player.addComponent<WeaponComponent>("rifle", 30);

    vshade::scene::Entity enemy = source.createEntity("Enemy");
    enemy.addComponent<HealthComponent>(50);
    enemy.addComponent<WeaponComponent>("sword", 0);

    // Components are ordinary mutable structs.
    player.component<HealthComponent>().points = 80;
    player.component<WeaponComponent>().ammunition = 29;
    enemy.component<HealthComponent>().points = 40;

    const std::uint64_t playerUuid = player.uuid();
    const std::uint64_t enemyUuid = enemy.uuid();
    const std::filesystem::path path = sceneOutputPath("basic-scene.json");

    vshade::scene::SceneSerializer writer(source);
    REQUIRE(writer.serialize(path, vshade::scene::SceneJsonFormat::Compact));
    const std::string compactJson =
        vshade::core::filesystem::readTextFile(path);
    CHECK(compactJson.find('\n') == std::string::npos);

    // Load the file into a completely new scene.
    vshade::scene::Scene loaded;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));

    const vshade::scene::Entity loadedPlayer = loaded.findEntity(playerUuid);
    const vshade::scene::Entity loadedEnemy = loaded.findEntity(enemyUuid);

    REQUIRE(loadedPlayer.valid());
    REQUIRE(loadedEnemy.valid());
    CHECK(loaded.name() == "Simple scene");
    CHECK(loadedPlayer.component<vshade::scene::TagComponent>().tag == "Player");
    CHECK(loadedPlayer.component<HealthComponent>().points == 80);
    CHECK(loadedPlayer.component<WeaponComponent>().definitionPath == "rifle");
    CHECK(loadedPlayer.component<WeaponComponent>().ammunition == 29);
    CHECK(loadedEnemy.component<vshade::scene::TagComponent>().tag == "Enemy");
    CHECK(loadedEnemy.component<HealthComponent>().points == 40);
    CHECK(loadedEnemy.component<WeaponComponent>().definitionPath == "sword");

}

TEST_CASE("Scene serialization rejects non-finite transforms with a diagnostic", "[scene]") {
    const std::filesystem::path path = sceneOutputPath("non-finite.json");
    vshade::scene::Scene scene;
    vshade::scene::Entity entity = scene.createEntity();
    entity.component<vshade::scene::TransformComponent>().transform.setPosition({
        std::numeric_limits<float>::infinity(),
        0.0F,
        0.0F,
    });

    vshade::scene::SceneSerializer serializer(scene);
    CHECK_FALSE(serializer.serialize(path));
    CHECK_FALSE(serializer.lastError().empty());
}

TEST_CASE("Scene files are serialized in stable UUID order", "[scene]") {
    const std::filesystem::path inputPath = sceneOutputPath("unsorted.json");
    const std::filesystem::path outputPath = sceneOutputPath("sorted.json");
    const auto entityJson = [](const std::string& uuid) {
        return nlohmann::json{
            {"Entity", uuid},
            {"Tag", "Entity " + uuid},
            {"Transform", {
                {"Position", {0.0, 0.0, 0.0}},
                {"Rotation", {0.0, 0.0, 0.0, 1.0}},
                {"Scale", {1.0, 1.0, 1.0}},
            }},
        };
    };
    const nlohmann::json input{
        {"FormatVersion", 2},
        {"Scene", "Ordering"},
        {"Entities", {entityJson("200"), entityJson("100")}},
    };
    {
        std::ofstream output(inputPath, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output << input;
    }

    vshade::scene::Scene scene;
    vshade::scene::SceneSerializer serializer(scene);
    REQUIRE(serializer.deserialize(inputPath));
    REQUIRE(serializer.serialize(outputPath));
    {
        std::ifstream output(outputPath, std::ios::binary);
        REQUIRE(output);
        const nlohmann::json serialized = nlohmann::json::parse(output);
        CHECK(serialized.at("Entities").at(0).at("Entity") == "100");
        CHECK(serialized.at("Entities").at(1).at("Entity") == "200");
    }

}

TEST_CASE("Scene format one Euler rotations migrate to quaternions", "[scene]") {
    const std::filesystem::path path = sceneOutputPath("format-v1.json");
    const nlohmann::json legacy{
        {"FormatVersion", 1},
        {"Scene", "Legacy"},
        {"Entities", {nlohmann::json{
            {"Entity", "42"},
            {"Tag", "Legacy entity"},
            {"Transform", {
                {"Position", {0.0, 0.0, 0.0}},
                {"Rotation", {0.1, 0.2, 0.3}},
                {"Scale", {1.0, 1.0, 1.0}},
            }},
        }}},
    };
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        REQUIRE(output);
        output << legacy;
    }

    vshade::scene::Scene scene;
    vshade::scene::SceneSerializer serializer(scene);
    REQUIRE(serializer.deserialize(path));
    const vshade::scene::Entity entity = scene.findEntity(42);
    REQUIRE(entity.valid());
    const auto expected = vshade::math::fromEuler({0.1F, 0.2F, 0.3F});
    CHECK(entity.component<vshade::scene::TransformComponent>().transform.rotation().w ==
          Catch::Approx(expected.w));

}

TEST_CASE("Scene serialization matches its golden JSON file", "[scene]") {
    const std::filesystem::path goldenPath =
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "scene" / "scene_example.json";
    const std::filesystem::path actualPath = sceneOutputPath("golden-actual.json");

    vshade::scene::Scene scene;
    vshade::scene::SceneSerializer serializer(scene);
    REQUIRE(serializer.deserialize(goldenPath));
    REQUIRE(serializer.serialize(actualPath));

    const nlohmann::json expected = nlohmann::json::parse(
        vshade::core::filesystem::readTextFile(goldenPath)
    );
    const nlohmann::json actual = nlohmann::json::parse(
        vshade::core::filesystem::readTextFile(actualPath)
    );
    CHECK(actual == expected);

}
