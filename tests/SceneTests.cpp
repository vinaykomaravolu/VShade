#include <catch2/catch_test_macros.hpp>

#include <core/Filesystem.hpp>
#include <asset/AssetManager.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/Prefab.hpp>
#include <scene/SceneComponentRegistry.hpp>
#include <scene/SceneSerializer.hpp>

#include <catch2/catch_approx.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <variant>

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
    CHECK(scene.findEntityById(player.id()) == player);
    CHECK(player.hasComponents<
        vshade::scene::TagComponent,
        vshade::scene::TransformComponent
    >());
    CHECK(player.component<vshade::scene::TagComponent>().tag == "Player");

    player.addComponent<HealthComponent>(75);
    CHECK(player.component<HealthComponent>().points == 75);
    CHECK(std::string(player.name()) == "Player");
    player.setName("Hero");
    CHECK(std::string(player.name()) == "Hero");
    player.transform().setPosition({1.0F, 2.0F, 3.0F});
    CHECK(player.transform().position().y == 2.0F);
    player.set<HealthComponent>(90);
    CHECK(player.get<HealthComponent>().points == 90);
    CHECK(player.tryGet<HealthComponent>() != nullptr);
    CHECK(player.has<HealthComponent>());
    CHECK(player.remove<HealthComponent>());
    CHECK_FALSE(player.has<HealthComponent>());

    std::size_t entityCount = 0;
    for (const auto handle : scene.view<vshade::scene::TransformComponent>()) {
        (void)handle;
        ++entityCount;
    }
    CHECK(entityCount == 1);

    scene.destroyEntity(player);
    CHECK_FALSE(player.valid());
    CHECK_FALSE(scene.findEntityById(player.id()));
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

TEST_CASE("Scene hierarchy rejects cycles and survives instancing", "[scene][hierarchy]") {
    vshade::scene::Scene scene("Hierarchy");
    auto root = scene.create("Root");
    auto child = scene.create("Child");
    auto grandchild = scene.create("Grandchild");
    scene.setParent(child, root);
    scene.setParent(grandchild, child);

    CHECK(scene.parent(child) == root);
    REQUIRE(scene.children(root).size() == 1);
    CHECK(scene.children(root)[0] == child);
    CHECK_THROWS_AS(scene.setParent(root, grandchild), std::invalid_argument);

    auto instance = scene.instantiate();
    const auto instanceChild = instance->findEntity(child.uuid());
    CHECK(instance->parent(instanceChild).uuid() == root.uuid());

    const auto path = sceneOutputPath("hierarchy.json");
    vshade::scene::SceneSerializer writer(scene);
    REQUIRE(writer.serialize(path));
    vshade::scene::Scene loaded;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));
    CHECK(loaded.parent(loaded.findEntity(grandchild.uuid())).uuid() == child.uuid());

    scene.destroyEntity(root);
    CHECK_FALSE(scene.parent(child));
}

TEST_CASE("Prefab instantiation remaps entity identity and hierarchy", "[scene][prefab]") {
    auto templateScene = std::make_shared<vshade::scene::Scene>("Ball prefab");
    auto root = templateScene->create("Ball");
    auto child = templateScene->create("Light");
    templateScene->setParent(child, root);
    root.add<vshade::scene::LightComponent>();
    const std::uint64_t templateRootUuid = root.uuid();

    vshade::scene::Prefab prefab(templateScene);
    vshade::scene::Scene destination("Level");
    const auto first = destination.instantiate(prefab);
    const auto second = destination.instantiate(prefab);

    REQUIRE(first);
    REQUIRE(second);
    CHECK(first.uuid() != templateRootUuid);
    CHECK(first.uuid() != second.uuid());
    CHECK(first.has<vshade::scene::LightComponent>());
    REQUIRE(destination.children(first).size() == 1);
    CHECK(destination.parent(destination.children(first)[0]) == first);
}

TEST_CASE("Prefab extraction copies a subtree and can round-trip through JSON", "[scene][prefab]") {
    vshade::scene::Scene source("Level");
    auto root = source.create("Turret");
    auto child = source.create("Barrel");
    auto ignored = source.create("Decor");
    source.setParent(child, root);
    root.transform().setPosition({1.0F, 2.0F, 3.0F});
    child.transform().setPosition({0.0F, 1.0F, 0.0F});
    ignored.transform().setPosition({9.0F, 9.0F, 9.0F});
    root.add<vshade::scene::LightComponent>();

    const auto prefab = vshade::scene::Prefab::fromEntity(source, root);
    CHECK(prefab.scene().name() == "Turret");
    std::size_t prefabEntityCount = 0;
    for (const auto handle : prefab.scene().view<vshade::scene::TagComponent>()) {
        static_cast<void>(handle);
        ++prefabEntityCount;
    }
    CHECK(prefabEntityCount == 2);

    vshade::scene::Scene destination("Spawned");
    const auto instance = destination.instantiate(prefab);
    REQUIRE(instance);
    CHECK(std::string(instance.name()) == "Turret");
    CHECK(instance.has<vshade::scene::LightComponent>());
    CHECK(instance.transform().position().x == Catch::Approx(1.0F));
    REQUIRE(destination.children(instance).size() == 1);
    CHECK(std::string(destination.children(instance)[0].name()) == "Barrel");

    const std::filesystem::path path = sceneOutputPath("turret.vsprefab");
    REQUIRE(prefab.save(path));

    vshade::asset::AssetManager assets;
    const auto loaded = assets.loadResource<vshade::scene::Prefab>(path);
    REQUIRE(loaded);
    vshade::scene::Scene loadedDestination("Loaded");
    const auto loadedInstance = loadedDestination.instantiate(*loaded);
    REQUIRE(loadedInstance);
    CHECK(loadedInstance.has<vshade::scene::LightComponent>());
    REQUIRE(loadedDestination.children(loadedInstance).size() == 1);
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
    original.addComponent<vshade::scene::AudioListenerComponent>(false);
    original.addComponent<vshade::scene::AudioSourceComponent>(
        vshade::audio::AudioClipHandle::fromId(42),
        vshade::audio::AudioBus::SFX,
        vshade::audio::AudioLoadMode::Decode,
        0.75F,
        1.0F,
        false,
        false,
        true
    );
    original.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::PointLight{
            .position = {1.0F, 2.0F, 3.0F},
            .color = {0.4F, 0.6F, 1.0F},
            .intensity = 2.0F,
            .range = 8.0F,
        },
        false
    );
    original.addComponent<vshade::scene::RigidBody2DComponent>(
        vshade::physics::PhysicsBody2DSettings{
            .type = vshade::physics::BodyType::Kinematic,
            .linearVelocity = {2.0F, 3.0F},
        }
    );
    original.addComponent<vshade::scene::Collider2DComponent>(
        vshade::physics::CircleShape2D{0.75F},
        vshade::physics::PhysicsMaterial2D{.density = 2.0F},
        vshade::math::Vec2{0.1F, 0.2F},
        true
    );
    original.addComponent<vshade::scene::RigidBody3DComponent>(
        vshade::physics::PhysicsBody3DSettings{
            .type = vshade::physics::BodyType::Dynamic,
            .linearVelocity = {4.0F, 5.0F, 6.0F},
        }
    );
    original.addComponent<vshade::scene::Collider3DComponent>(
        vshade::physics::SphereShape3D{1.25F},
        vshade::physics::PhysicsMaterial3D{.friction = 0.8F},
        vshade::math::Vec3{0.3F, 0.4F, 0.5F},
        false
    );

    const vshade::scene::Entity duplicate = scene.duplicateEntity(original);
    REQUIRE(duplicate.valid());
    CHECK(duplicate.uuid() != original.uuid());
    CHECK(duplicate.component<vshade::scene::TagComponent>().tag == "Player Copy");
    CHECK(duplicate.component<vshade::scene::TransformComponent>().transform.position().x ==
          Catch::Approx(1.0F));
    CHECK(duplicate.component<vshade::scene::SpriteRendererComponent>().texturePath ==
          playerTexturePath);
    CHECK_FALSE(duplicate.component<vshade::scene::AudioListenerComponent>().active);
    const auto& duplicatedAudio =
        duplicate.component<vshade::scene::AudioSourceComponent>();
    CHECK(duplicatedAudio.clip.id() == 42);
    CHECK(duplicatedAudio.spatial);
    const auto& duplicatedLight =
        duplicate.component<vshade::scene::LightComponent>();
    CHECK_FALSE(duplicatedLight.enabled);
    REQUIRE(std::holds_alternative<vshade::renderer::PointLight>(duplicatedLight.light));
    CHECK(std::get<vshade::renderer::PointLight>(duplicatedLight.light).range ==
          Catch::Approx(8.0F));
    CHECK(duplicate.component<vshade::scene::RigidBody2DComponent>().settings.type ==
          vshade::physics::BodyType::Kinematic);
    CHECK(std::get<vshade::physics::CircleShape2D>(
              duplicate.component<vshade::scene::Collider2DComponent>().shape
          ).radius == Catch::Approx(0.75F));
    CHECK(duplicate.component<vshade::scene::RigidBody3DComponent>()
              .settings.linearVelocity.z == Catch::Approx(6.0F));
    CHECK(std::get<vshade::physics::SphereShape3D>(
              duplicate.component<vshade::scene::Collider3DComponent>().shape
          ).radius == Catch::Approx(1.25F));
}

TEST_CASE("Scene physics components round trip through JSON", "[scene][physics]") {
    const std::filesystem::path path = sceneOutputPath("physics-components.json");
    vshade::scene::Scene source("Physics persistence");

    vshade::scene::Entity entity2D = source.createEntity("2D body");
    const std::uint64_t uuid2D = entity2D.uuid();
    entity2D.addComponent<vshade::scene::RigidBody2DComponent>(
        vshade::physics::PhysicsBody2DSettings{
            .type = vshade::physics::BodyType::Dynamic,
            .linearVelocity = {3.0F, -2.0F},
            .angularVelocity = 1.5F,
            .linearDamping = 0.2F,
            .angularDamping = 0.3F,
            .gravityScale = 0.75F,
            .fixedRotation = true,
            .continuousCollision = true,
            .enabled = false,
            .collision = {.layer = 4, .mask = 12},
        }
    );
    entity2D.addComponent<vshade::scene::Collider2DComponent>(
        vshade::physics::CapsuleShape2D{.halfHeight = 1.25F, .radius = 0.4F},
        vshade::physics::PhysicsMaterial2D{
            .density = 2.0F,
            .friction = 0.25F,
            .restitution = 0.6F,
        },
        vshade::math::Vec2{0.1F, -0.2F},
        true
    );

    vshade::scene::Entity entity3D = source.createEntity("3D body");
    const std::uint64_t uuid3D = entity3D.uuid();
    entity3D.addComponent<vshade::scene::RigidBody3DComponent>(
        vshade::physics::PhysicsBody3DSettings{
            .type = vshade::physics::BodyType::Kinematic,
            .linearVelocity = {1.0F, 2.0F, 3.0F},
            .angularVelocity = {0.1F, 0.2F, 0.3F},
            .mass = 4.0F,
            .linearDamping = 0.4F,
            .angularDamping = 0.5F,
            .gravityScale = -0.25F,
            .continuousCollision = true,
            .enabled = false,
            .collision = {.layer = 8, .mask = 16},
        }
    );
    entity3D.addComponent<vshade::scene::Collider3DComponent>(
        vshade::physics::BoxShape3D{{1.0F, 2.0F, 3.0F}},
        vshade::physics::PhysicsMaterial3D{
            .friction = 0.7F,
            .restitution = 0.2F,
        },
        vshade::math::Vec3{0.3F, 0.4F, 0.5F},
        false
    );

    vshade::scene::SceneSerializer writer(source);
    REQUIRE(writer.serialize(path));
    vshade::scene::Scene loaded;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));

    const auto loaded2D = loaded.findEntity(uuid2D);
    REQUIRE(loaded2D.hasComponents<
        vshade::scene::RigidBody2DComponent,
        vshade::scene::Collider2DComponent
    >());
    const auto& body2D = loaded2D.component<vshade::scene::RigidBody2DComponent>().settings;
    CHECK(body2D.type == vshade::physics::BodyType::Dynamic);
    CHECK(body2D.linearVelocity.x == Catch::Approx(3.0F));
    CHECK(body2D.angularVelocity == Catch::Approx(1.5F));
    CHECK(body2D.fixedRotation);
    CHECK_FALSE(body2D.enabled);
    CHECK(body2D.collision.layer == 4);
    const auto& collider2D = loaded2D.component<vshade::scene::Collider2DComponent>();
    REQUIRE(std::holds_alternative<vshade::physics::CapsuleShape2D>(collider2D.shape));
    CHECK(std::get<vshade::physics::CapsuleShape2D>(collider2D.shape).halfHeight ==
          Catch::Approx(1.25F));
    CHECK(collider2D.material.restitution == Catch::Approx(0.6F));
    CHECK(collider2D.sensor);

    const auto loaded3D = loaded.findEntity(uuid3D);
    REQUIRE(loaded3D.hasComponents<
        vshade::scene::RigidBody3DComponent,
        vshade::scene::Collider3DComponent
    >());
    const auto& body3D = loaded3D.component<vshade::scene::RigidBody3DComponent>().settings;
    CHECK(body3D.type == vshade::physics::BodyType::Kinematic);
    CHECK(body3D.angularVelocity.z == Catch::Approx(0.3F));
    CHECK(body3D.mass == Catch::Approx(4.0F));
    CHECK(body3D.gravityScale == Catch::Approx(-0.25F));
    CHECK(body3D.collision.mask == 16);
    const auto& collider3D = loaded3D.component<vshade::scene::Collider3DComponent>();
    REQUIRE(std::holds_alternative<vshade::physics::BoxShape3D>(collider3D.shape));
    CHECK(std::get<vshade::physics::BoxShape3D>(collider3D.shape).halfExtents.y ==
          Catch::Approx(2.0F));
    CHECK(collider3D.offset.z == Catch::Approx(0.5F));
}

TEST_CASE("Scene serialization round trips stable components", "[scene]") {
    const std::filesystem::path path = sceneOutputPath("roundtrip.json");

    vshade::scene::Scene source("Example");
    source.setEnvironment({
        .ambientColor = {0.25F, 0.5F, 0.75F},
        .ambientIntensity = 0.2F,
    });
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
    CHECK(loaded.environment().ambientColor.g == Catch::Approx(0.5F));
    CHECK(loaded.environment().ambientIntensity == Catch::Approx(0.2F));
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

TEST_CASE("Scene instantiation preserves identity without sharing mutable state", "[scene]") {
    vshade::scene::Scene sceneAsset("Instanced level");
    sceneAsset.setEnvironment({.ambientIntensity = 0.4F});
    vshade::scene::Entity source = sceneAsset.createEntity("Template ball");
    const std::uint64_t uuid = source.uuid();
    source.component<vshade::scene::TransformComponent>().transform.setPosition(
        {1.0F, 2.0F, 3.0F}
    );
    source.addComponent<vshade::scene::RigidBody3DComponent>(
        vshade::physics::PhysicsBody3DSettings{
            .type = vshade::physics::BodyType::Dynamic,
        }
    );
    source.addComponent<vshade::scene::Collider3DComponent>(
        vshade::physics::SphereShape3D{0.5F}
    );

    std::unique_ptr<vshade::scene::Scene> first = sceneAsset.instantiate();
    std::unique_ptr<vshade::scene::Scene> second = sceneAsset.instantiate();
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->name() == "Instanced level");
    CHECK(first->environment().ambientIntensity == Catch::Approx(0.4F));

    auto firstBall = first->findEntity(uuid);
    auto secondBall = second->findEntity(uuid);
    REQUIRE(firstBall.valid());
    REQUIRE(secondBall.valid());
    REQUIRE(firstBall.hasComponents<
        vshade::scene::RigidBody3DComponent,
        vshade::scene::Collider3DComponent
    >());
    firstBall.component<vshade::scene::TransformComponent>().transform.setPosition(
        {9.0F, 9.0F, 9.0F}
    );
    CHECK(secondBall.component<vshade::scene::TransformComponent>()
              .transform.position().x == Catch::Approx(1.0F));
    CHECK(source.component<vshade::scene::TransformComponent>()
              .transform.position().x == Catch::Approx(1.0F));
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
    const auto result = serializer.deserializeResult(path);
    REQUIRE_FALSE(result);
    CHECK(result.error().code == "scene.deserialize");
    CHECK(result.error().path == path);
    CHECK(scene.name() == "Keep me");
    CHECK(scene.findEntity(survivorUuid).valid());

}

TEST_CASE("Scene serialization round trips audio environment and entity lights", "[scene][audio][lighting]") {
    const std::filesystem::path path = sceneOutputPath("new-components.json");
    vshade::scene::Scene source("Audio and lighting");
    source.setEnvironment({
        .ambientColor = {0.2F, 0.3F, 0.5F},
        .ambientIntensity = 0.25F,
    });

    vshade::scene::Entity audio = source.createEntity("Music source");
    const std::uint64_t audioUuid = audio.uuid();
    vshade::asset::AssetManager assets;
    const auto audioReference = assets.reference<vshade::audio::AudioClip>(
        "audio/theme.flac"
    );
    auto& sourceComponent = audio.addComponent<vshade::scene::AudioSourceComponent>(
        audioReference.handle(),
        vshade::audio::AudioBus::Music,
        vshade::audio::AudioLoadMode::Stream,
        0.65F,
        1.1F,
        true,
        true,
        false
    );
    sourceComponent.clipAsset = audioReference;
    sourceComponent.attenuation = vshade::audio::AttenuationModel::Linear;
    sourceComponent.minimumDistance = 2.0F;
    sourceComponent.maximumDistance = 30.0F;
    audio.addComponent<vshade::scene::AudioListenerComponent>(false);

    vshade::scene::Entity directional = source.createEntity("Directional light");
    const std::uint64_t directionalUuid = directional.uuid();
    directional.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::DirectionalLight{
            .direction = {0.0F, -1.0F, 0.0F},
            .color = {1.0F, 0.8F, 0.6F},
            .intensity = 1.5F,
        },
        false
    );

    vshade::scene::Entity point = source.createEntity("Point light");
    const std::uint64_t pointUuid = point.uuid();
    point.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::PointLight{
            .position = {2.0F, 3.0F, 4.0F},
            .color = {0.1F, 0.4F, 1.0F},
            .intensity = 3.0F,
            .range = 12.0F,
        },
        true
    );

    vshade::scene::SceneSerializer writer(source);
    REQUIRE(writer.serialize(path));

    const nlohmann::json serialized = nlohmann::json::parse(
        vshade::core::filesystem::readTextFile(path)
    );
    REQUIRE(serialized.at("Entities").size() == 3);
    CHECK(serialized.at("Environment").at("AmbientIntensity").get<double>() ==
          Catch::Approx(0.25));

    vshade::scene::Scene loaded;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));

    const auto loadedAudio = loaded.findEntity(audioUuid);
    REQUIRE(loadedAudio.valid());
    const auto& audioSource =
        loadedAudio.component<vshade::scene::AudioSourceComponent>();
    CHECK(audioSource.clip == audioReference.handle());
    CHECK(audioSource.bus == vshade::audio::AudioBus::Music);
    CHECK(audioSource.loadMode == vshade::audio::AudioLoadMode::Stream);
    CHECK(audioSource.volume == Catch::Approx(0.65F));
    CHECK(audioSource.pitch == Catch::Approx(1.1F));
    CHECK(audioSource.looping);
    CHECK(audioSource.playOnStart);
    CHECK_FALSE(audioSource.spatial);
    CHECK(audioSource.clipAsset.sourcePath() == "audio/theme.flac");
    CHECK(audioSource.attenuation == vshade::audio::AttenuationModel::Linear);
    CHECK(audioSource.minimumDistance == Catch::Approx(2.0F));
    CHECK(audioSource.maximumDistance == Catch::Approx(30.0F));
    CHECK_FALSE(
        loadedAudio.component<vshade::scene::AudioListenerComponent>().active
    );

    CHECK(loaded.environment().ambientColor.b == Catch::Approx(0.5F));
    CHECK(loaded.environment().ambientIntensity == Catch::Approx(0.25F));

    const auto& directionalLight = loaded.findEntity(directionalUuid)
        .component<vshade::scene::LightComponent>();
    CHECK_FALSE(directionalLight.enabled);
    REQUIRE(std::holds_alternative<vshade::renderer::DirectionalLight>(
        directionalLight.light
    ));
    CHECK(std::get<vshade::renderer::DirectionalLight>(directionalLight.light)
              .direction.y == Catch::Approx(-1.0F));

    const auto& pointLight =
        loaded.findEntity(pointUuid).component<vshade::scene::LightComponent>();
    REQUIRE(std::holds_alternative<vshade::renderer::PointLight>(pointLight.light));
    CHECK(std::get<vshade::renderer::PointLight>(pointLight.light).position.z ==
          Catch::Approx(4.0F));
    CHECK(std::get<vshade::renderer::PointLight>(pointLight.light).range ==
          Catch::Approx(12.0F));
}

TEST_CASE("Scene serialization rejects invalid new component values", "[scene][audio][lighting]") {
    const std::filesystem::path path = sceneOutputPath("invalid-new-component.json");
    vshade::scene::Scene scene;
    vshade::scene::Entity entity = scene.createEntity("Invalid light");
    entity.addComponent<vshade::scene::LightComponent>(
        vshade::renderer::PointLight{.range = 0.0F},
        true
    );

    vshade::scene::SceneSerializer serializer(scene);
    CHECK_FALSE(serializer.serialize(path));
    CHECK_FALSE(serializer.lastError().empty());
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
