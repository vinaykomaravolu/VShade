#include "assets/Bounce.hpp"
#include "assets/RotatePlatform.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Application.hpp>
#include <core/EngineServices.hpp>
#include <scene/Components.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneComponentRegistry.hpp>
#include <scene/SceneRuntime.hpp>
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

class ContextScript final : public vshade::script::NativeScript {
public:
    void onCreate() override {
        receivedServices = context().hasServices();
        sceneName = context().scene().name();
        spawned = context().scene().create("Spawned by script").valid();
        physicsAvailable = &context().physics3D() == &context().runtime().physics3D();
    }

    static inline bool receivedServices = false;
    static inline bool spawned = false;
    static inline bool physicsAvailable = false;
    static inline std::string sceneName;
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
          m_runtime(m_registry) {}

    [[nodiscard]] const vshade::scene::Scene& scene() const noexcept {
        return m_scene;
    }

    [[nodiscard]] bool shutdownCalled() const noexcept {
        return m_shutdownCalled;
    }

    [[nodiscard]] bool scriptsDetached() const noexcept {
        return !m_runtime.isPlaying() &&
            !m_runtime.nativeScripts().hasAttachedScene();
    }

protected:
    void onStart() override {
        InlineMovementScript::reset();
        m_registry.registerType<InlineMovementScript>("InlineMovement");
        vshade::scene::Entity entity = m_scene.createEntity("Application script");
        entity.addComponent<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{{.typeName = "InlineMovement"}}}
        );
        m_runtime.play(m_scene);
    }

    void onUpdate(const float deltaTime) override {
        m_runtime.update(deltaTime);
        ++m_applicationUpdateCount;
        if (InlineMovementScript::fixedUpdateCount > 0 ||
            m_applicationUpdateCount >= 1000) {
            close();
        }
    }

    void onFixedUpdate(const float fixedDeltaTime) override {
        m_runtime.fixedUpdate(fixedDeltaTime);
    }

    void onShutdown() override {
        m_runtime.stop();
        m_shutdownCalled = true;
    }

private:
    vshade::script::NativeScriptRegistry m_registry;
    vshade::scene::Scene m_scene{"Scripted application"};
    vshade::scene::SceneRuntime m_runtime;
    std::size_t m_applicationUpdateCount = 0;
    bool m_shutdownCalled = false;
};

class AutomaticRuntimeApplicationTest final : public vshade::core::Application {
public:
    AutomaticRuntimeApplicationTest()
        : Application({
              .window = {
                  .title = "Automatic scene runtime test",
                  .width = 64,
                  .height = 64,
                  .vsync = false,
                  .visible = false,
              },
              .audio = {.enableDevice = false},
          }) {}

    [[nodiscard]] const vshade::scene::Scene& testScene() const noexcept {
        return m_scene;
    }

protected:
    void onStart() override {
        InlineMovementScript::reset();
        scripts().registerType<InlineMovementScript>("AutomaticMovement");
        m_scene.create("Actor").add<vshade::scene::ScriptComponent>(
            vshade::scene::ScriptComponent{{{.typeName = "AutomaticMovement"}}}
        );
        playScene(m_scene);
    }

    void onUpdate(float) override {
        if (InlineMovementScript::updateCount > 0) close();
    }

private:
    vshade::scene::Scene m_scene{"Automatic runtime"};
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

TEST_CASE("Application automatically forwards its active SceneRuntime",
          "[scripting][application][scene-runtime][opengl]") {
    AutomaticRuntimeApplicationTest application;
    REQUIRE(application.run() == 0);
    CHECK(InlineMovementScript::createCount == 1);
    CHECK(InlineMovementScript::updateCount > 0);
    CHECK(InlineMovementScript::destroyCount == 1);
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

TEST_CASE("SceneRuntime supplies scripts with typed gameplay context", "[scripting][scene-runtime]") {
    ContextScript::receivedServices = false;
    ContextScript::spawned = false;
    ContextScript::physicsAvailable = false;
    ContextScript::sceneName.clear();

    vshade::core::EngineServices services({.enableDevice = false});
    services.scripts().registerType<ContextScript>("ContextScript");
    vshade::scene::Scene scene("Context world");
    scene.create("Controller").add<vshade::scene::ScriptComponent>(
        vshade::scene::ScriptComponent{{{.typeName = "ContextScript"}}}
    );
    vshade::scene::SceneRuntime runtime(services, {
        .physics2D = false,
        .physics3D = false,
        .rendering = false,
        .audio = false,
    });

    runtime.play(scene);
    CHECK(ContextScript::receivedServices);
    CHECK(ContextScript::spawned);
    CHECK(ContextScript::physicsAvailable);
    CHECK(ContextScript::sceneName == "Context world");
    runtime.stop();
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
    REQUIRE(system.diagnostics().size() == 1);
    CHECK(system.diagnostics()[0].code == "script.backend_unavailable");
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

TEST_CASE("SceneRuntime owns one explicit play-mode lifecycle", "[scripting][scene-runtime]") {
    vshade::script::NativeScriptRegistry registry;
    vshade::scene::Scene scene("Runtime lifecycle");
    vshade::scene::SceneRuntime runtime(registry);

    CHECK_FALSE(runtime.isPlaying());
    CHECK(runtime.scene() == nullptr);
    CHECK_THROWS_AS(runtime.update(0.0F), std::logic_error);
    CHECK_THROWS_AS(runtime.fixedUpdate(0.0F), std::logic_error);

    runtime.play(scene);
    CHECK(runtime.isPlaying());
    CHECK_FALSE(runtime.isPaused());
    CHECK(runtime.scene() == &scene);
    CHECK(runtime.nativeScripts().hasAttachedScene());
    CHECK_THROWS_AS(runtime.play(scene), std::logic_error);

    runtime.setPaused(true);
    CHECK(runtime.isPaused());
    runtime.setPaused(false);
    CHECK_FALSE(runtime.isPaused());
    runtime.update(0.0F);
    runtime.fixedUpdate(0.001F);
    runtime.stop();
    runtime.stop();
    CHECK_FALSE(runtime.isPlaying());
    CHECK_FALSE(runtime.isPaused());
    CHECK(runtime.scene() == nullptr);
    CHECK_FALSE(runtime.nativeScripts().hasAttachedScene());
}

TEST_CASE("SceneRuntime invokes user systems at deterministic phases", "[scene-runtime]") {
    vshade::script::NativeScriptRegistry registry;
    vshade::scene::Scene scene("Phases");
    vshade::scene::SceneRuntime runtime(registry);
    std::vector<std::string> phases;
    const auto record = [&phases](const char* name) {
        return [&phases, name](vshade::scene::Scene&, float) {
            phases.emplace_back(name);
        };
    };
    runtime.addSystem(vshade::scene::SceneRuntimePhase::BeforeUpdate, record("before update"));
    runtime.addSystem(vshade::scene::SceneRuntimePhase::AfterUpdate, record("after update"));
    runtime.addSystem(vshade::scene::SceneRuntimePhase::BeforePhysics, record("before physics"));
    runtime.addSystem(vshade::scene::SceneRuntimePhase::AfterPhysics, record("after physics"));
    runtime.addSystem(vshade::scene::SceneRuntimePhase::BeforeRender, record("before render"));
    runtime.addSystem(vshade::scene::SceneRuntimePhase::AfterRender, record("after render"));

    runtime.play(scene);
    runtime.update(0.1F);
    runtime.fixedUpdate(0.02F);
    CHECK_FALSE(runtime.render(64, 64));
    CHECK(phases == std::vector<std::string>{
        "before update", "after update", "before physics",
        "after physics", "before render", "after render",
    });
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
