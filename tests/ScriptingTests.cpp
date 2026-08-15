#include "assets/Bounce.hpp"
#include "assets/RotatePlatform.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Application.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneComponentRegistry.hpp>
#include <scene/SceneSerializer.hpp>
#include <script/Scripting.hpp>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

class InlineMovementScript final : public vshade::script::NativeScript {
public:
    void onCreate() override {
        ++createCount;
        createdWithEntity = entity().valid();
    }

    void onUpdate(const float deltaTime) override {
        ++updateCount;
        auto& transform = component<vshade::scene::TransformComponent>().transform;
        transform.translate({deltaTime, 0.0F, 0.0F});
    }

    void onFixedUpdate(const float fixedDeltaTime) override {
        ++fixedUpdateCount;
        auto& transform = component<vshade::scene::TransformComponent>().transform;
        transform.translate({0.0F, fixedDeltaTime, 0.0F});
    }

    void onDestroy() override {
        ++destroyCount;
    }

    static void reset() noexcept {
        createCount = 0;
        updateCount = 0;
        fixedUpdateCount = 0;
        destroyCount = 0;
        createdWithEntity = false;
    }

    static inline int createCount = 0;
    static inline int updateCount = 0;
    static inline int fixedUpdateCount = 0;
    static inline int destroyCount = 0;
    static inline bool createdWithEntity = false;
};

class ScriptedApplicationTest final : public vshade::core::Application {
public:
    ScriptedApplicationTest()
        : Application({
              .window = {
                  .title = "VShade scripted application test",
                  .width = 64,
                  .height = 64,
                  .fullscreen = false,
                  .vsync = false,
                  .visible = false,
              },
              .fixedDeltaTime = 0.001F,
              .maximumDeltaTime = 0.05F,
          }),
          m_scripts(m_registry) {}

    [[nodiscard]] const vshade::scene::Scene& scene() const noexcept {
        return m_scene;
    }

    [[nodiscard]] bool shutdownCalled() const noexcept {
        return m_shutdownCalled;
    }

    [[nodiscard]] bool scriptsDetached() const noexcept {
        return !m_scripts.hasAttachedScene();
    }

protected:
    void onStart() override {
        InlineMovementScript::reset();
        m_registry.registerType<InlineMovementScript>("InlineMovement");
        vshade::scene::Entity entity = m_scene.createEntity("Application script");
        entity.addComponent<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{{.typeName = "InlineMovement"}}}
        );
        m_scripts.attachScene(m_scene);
    }

    void onUpdate(const float deltaTime) override {
        m_scripts.update(deltaTime);
        ++m_applicationUpdateCount;
        if (InlineMovementScript::fixedUpdateCount > 0 ||
            m_applicationUpdateCount >= 1000) {
            close();
        }
    }

    void onFixedUpdate(const float fixedDeltaTime) override {
        m_scripts.fixedUpdate(fixedDeltaTime);
    }

    void onShutdown() override {
        m_scripts.detachScene();
        m_shutdownCalled = true;
    }

private:
    vshade::script::NativeScriptRegistry m_registry;
    vshade::scene::Scene m_scene{"Scripted application"};
    vshade::script::NativeScriptSystem m_scripts;
    std::size_t m_applicationUpdateCount = 0;
    bool m_shutdownCalled = false;
};

} // namespace

TEST_CASE("ScriptedApplicationTest dispatches scripts through application hooks",
          "[scripting][application][opengl]") {
    ScriptedApplicationTest application;
    REQUIRE(application.run() == 0);

    CHECK(application.shutdownCalled());
    CHECK(application.scriptsDetached());
    CHECK(InlineMovementScript::createCount == 1);
    CHECK(InlineMovementScript::updateCount > 0);
    CHECK(InlineMovementScript::fixedUpdateCount > 0);
    CHECK(InlineMovementScript::destroyCount == 1);

    const auto view = application.scene().view<
        const vshade::scene::TransformComponent,
        const vshade::scene::ScriptComponent
    >();
    REQUIRE(view.size_hint() == 1);
    for (const auto [handle, transform, scripts] : view.each()) {
        (void)handle;
        CHECK(transform.transform.position().x > 0.0F);
        CHECK(transform.transform.position().y > 0.0F);
        CHECK(scripts.scripts.size() == 1);
    }
}

TEST_CASE("Native script registry creates user script classes", "[scripting]") {
    vshade::script::NativeScriptRegistry registry;
    registry.registerType<InlineMovementScript>("InlineMovement");
    registry.registerType<vshade::tests::assets::RotatePlatform>("RotatePlatform");
    registry.registerType<vshade::tests::assets::Bounce>("Bounce");

    CHECK(registry.contains("InlineMovement"));
    CHECK((registry.typeNames() == std::vector<std::string>{
        "Bounce", "InlineMovement", "RotatePlatform"
    }));
    CHECK(dynamic_cast<InlineMovementScript*>(registry.create("InlineMovement").get()) != nullptr);
    CHECK_THROWS_AS(
        registry.registerType<InlineMovementScript>("InlineMovement"),
        std::invalid_argument
    );
    CHECK_THROWS_AS(registry.create("Missing"), std::out_of_range);
    registry.registerFactory(
        "Null",
        [] { return std::unique_ptr<vshade::script::NativeScript>{}; }
    );
    CHECK_THROWS_AS(registry.create("Null"), std::runtime_error);
}

TEST_CASE("Native script system dispatches lifecycle and synchronizes bindings", "[scripting]") {
    InlineMovementScript::reset();
    vshade::script::NativeScriptRegistry registry;
    registry.registerType<InlineMovementScript>("InlineMovement");
    registry.registerType<vshade::tests::assets::RotatePlatform>("RotatePlatform");
    registry.registerType<vshade::tests::assets::Bounce>("Bounce");

    vshade::scene::Scene scene("Scripts");
    vshade::scene::Entity entity = scene.createEntity("Scripted entity");
    auto& component = entity.addComponent<vshade::scene::ScriptComponent>();
    component.scripts = {
        {.typeName = "InlineMovement", .enabled = true},
        {
            .backend = vshade::script::ScriptBackend::Lua,
            .typeName = "future.lua",
            .enabled = true,
        },
    };

    vshade::script::NativeScriptSystem system(registry);
    system.attachScene(scene);
    CHECK(system.hasAttachedScene());
    CHECK(system.instanceCount() == 1);
    CHECK(InlineMovementScript::createCount == 1);
    CHECK(InlineMovementScript::createdWithEntity);
    CHECK_THROWS_AS(system.attachScene(scene), std::logic_error);

    system.update(0.25F);
    system.fixedUpdate(0.02F);
    CHECK(InlineMovementScript::updateCount == 1);
    CHECK(InlineMovementScript::fixedUpdateCount == 1);
    CHECK(entity.component<vshade::scene::TransformComponent>().transform.position().x ==
          Catch::Approx(0.25F));
    CHECK(entity.component<vshade::scene::TransformComponent>().transform.position().y ==
          Catch::Approx(0.02F));

    component.scripts[0].enabled = false;
    system.update(0.25F);
    CHECK(InlineMovementScript::updateCount == 1);

    component.scripts.push_back({.typeName = "RotatePlatform", .enabled = true});
    component.scripts.push_back({.typeName = "Bounce", .enabled = true});
    system.update(0.5F);
    CHECK(system.instanceCount() == 3);
    CHECK(entity.component<vshade::scene::TransformComponent>().transform.position().y > 0.4F);

    entity.removeComponent<vshade::scene::ScriptComponent>();
    system.update(0.0F);
    CHECK(system.instanceCount() == 0);
    CHECK(InlineMovementScript::destroyCount == 1);

    system.detachScene();
    CHECK_FALSE(system.hasAttachedScene());
    CHECK_THROWS_AS(system.update(0.0F), std::logic_error);
}

TEST_CASE("Native script attachment rolls back an unknown type", "[scripting]") {
    vshade::script::NativeScriptRegistry registry;
    vshade::scene::Scene scene;
    vshade::scene::Entity entity = scene.createEntity();
    entity.addComponent<vshade::scene::ScriptComponent>(
        vshade::scene::ScriptComponent{{{.typeName = "Unknown"}}}
    );
    vshade::script::NativeScriptSystem system(registry);

    CHECK_THROWS_AS(system.attachScene(scene), std::out_of_range);
    CHECK_FALSE(system.hasAttachedScene());
    CHECK(system.instanceCount() == 0);
}

TEST_CASE("Script components duplicate and round-trip through scene JSON", "[scripting][scene]") {
    const std::filesystem::path path =
        std::filesystem::path(VSHADE_SCENE_OUTPUT_DIR) / "scripting-round-trip.json";
    std::filesystem::create_directories(path.parent_path());

    vshade::scene::Scene source("Scripting serialization");
    vshade::scene::Entity entity = source.createEntity("Scripted");
    entity.addComponent<vshade::scene::ScriptComponent>(
        vshade::scene::ScriptComponent{{
            {.typeName = "RotatePlatform", .enabled = true},
            {
                .backend = vshade::script::ScriptBackend::Lua,
                .typeName = "scripts/bounce.lua",
                .enabled = false,
            },
        }}
    );
    const vshade::scene::Entity duplicate = source.duplicateEntity(entity);
    REQUIRE(duplicate.hasComponents<vshade::scene::ScriptComponent>());
    CHECK(duplicate.component<vshade::scene::ScriptComponent>().scripts.size() == 2);

    vshade::scene::SceneSerializer writer(source);
    REQUIRE(writer.serialize(path));
    vshade::scene::Scene loaded;
    vshade::scene::SceneSerializer reader(loaded);
    REQUIRE(reader.deserialize(path));

    const auto view = loaded.view<const vshade::scene::TagComponent,
                                  const vshade::scene::ScriptComponent>();
    std::size_t scriptedEntityCount = 0;
    for (const auto [handle, tag, scripts] : view.each()) {
        (void)handle;
        (void)tag;
        ++scriptedEntityCount;
        REQUIRE(scripts.scripts.size() == 2);
        CHECK(scripts.scripts[0].typeName == "RotatePlatform");
        CHECK(scripts.scripts[0].backend == vshade::script::ScriptBackend::NativeCpp);
        CHECK(scripts.scripts[1].typeName == "scripts/bounce.lua");
        CHECK(scripts.scripts[1].backend == vshade::script::ScriptBackend::Lua);
        CHECK_FALSE(scripts.scripts[1].enabled);
    }
    CHECK(scriptedEntityCount == 2);
}

TEST_CASE("ScriptComponent is reserved as a built-in scene component", "[scripting][scene]") {
    CHECK_THROWS_AS(
        vshade::scene::SceneComponentRegistry::registerComponent<
            vshade::scene::ScriptComponent
        >("Scripts"),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        vshade::scene::SceneComponentRegistry::registerComponent<
            vshade::scene::ScriptComponent
        >("ScriptAlias"),
        std::invalid_argument
    );
}
