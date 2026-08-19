#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <asset/AssetManager.hpp>
#include <asset/AssetLoader.hpp>
#include <asset/AssetRegistry.hpp>
#include <asset/loader/MeshLoader.hpp>
#include <asset/loader/ModelLoader.hpp>
#include <asset/loader/SceneLoader.hpp>
#include <asset/loader/ShaderLoader.hpp>
#include <asset/loader/TextureLoader.hpp>

#include "visual/RenderFixture.hpp"

#include <renderer/Mesh.hpp>
#include <renderer/Material.hpp>
#include <renderer/Model.hpp>
#include <renderer/Shader.hpp>
#include <renderer/Texture.hpp>
#include <scene/Scene.hpp>
#include <scene/Prefab.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace {

struct TextAsset {
    std::string text;
};

struct OtherAsset {};

static_assert(std::is_abstract_v<vshade::asset::AssetLoader<TextAsset>>);
static_assert(std::is_base_of_v<
    vshade::asset::AssetLoader<vshade::renderer::Texture2D>,
    vshade::asset::TextureLoader
>);
static_assert(std::is_base_of_v<
    vshade::asset::AssetLoader<vshade::renderer::Shader>,
    vshade::asset::ShaderLoader
>);
static_assert(std::is_base_of_v<
    vshade::asset::AssetLoader<vshade::renderer::Mesh>,
    vshade::asset::MeshLoader
>);
static_assert(std::is_base_of_v<
    vshade::asset::AssetLoader<vshade::renderer::Model>,
    vshade::asset::ModelLoader
>);
static_assert(std::is_base_of_v<
    vshade::asset::AssetLoader<vshade::scene::Scene>,
    vshade::asset::SceneLoader
>);

} // namespace

TEST_CASE("Asset manager loads and reuses typed resources", "[asset]") {
    vshade::asset::AssetManager assets;
    int loadCount = 0;
    assets.registerLoader<TextAsset>([&loadCount](const std::filesystem::path& path) {
        ++loadCount;
        return std::make_shared<TextAsset>(TextAsset{path.generic_string()});
    });

    const auto first = assets.load<TextAsset>("assets/characters/../player.txt");
    const auto second = assets.load<TextAsset>("assets/player.txt");

    REQUIRE(first.valid());
    CHECK(first == second);
    CHECK(loadCount == 1);
    CHECK(assets.size() == 1);
    CHECK(assets.registeredAssetCount() == 1);
    CHECK(assets.isLoaded(first));

    const std::shared_ptr<TextAsset> resource = assets.get(first);
    REQUIRE(resource);
    CHECK(resource->text == "assets/player.txt");

    const auto metadata = assets.metadata(first);
    REQUIRE(metadata.has_value());
    CHECK(metadata->id == first.id());
    CHECK(metadata->sourcePath == std::filesystem::path("assets/player.txt"));

    REQUIRE(assets.unload(first));
    CHECK_FALSE(assets.isLoaded(first));
    CHECK_FALSE(assets.get(first));
    CHECK(assets.size() == 0);
    CHECK(assets.registeredAssetCount() == 1);
    CHECK(assets.metadata(first).has_value());

    assets.load(first);
    CHECK(assets.isLoaded(first));
    CHECK(loadCount == 2);

    // Existing shared owners remain alive after the manager releases its copy.
    CHECK(resource->text == "assets/player.txt");
}

TEST_CASE("Asset references resolve in a fresh manager and catalogs persist", "[asset]") {
    const std::filesystem::path catalog =
        std::filesystem::path(VSHADE_FILESYSTEM_OUTPUT_DIR) / "asset-catalog.json";
    std::filesystem::create_directories(catalog.parent_path());

    vshade::asset::AssetReference<TextAsset> reference;
    {
        vshade::asset::AssetManager assets;
        assets.registerLoader<TextAsset>([](const std::filesystem::path& path) {
            return std::make_shared<TextAsset>(TextAsset{path.generic_string()});
        });
        const auto resource = assets.loadResource<TextAsset>(
            "assets/characters/../player.txt"
        );
        REQUIRE(resource);
        CHECK(resource->text == "assets/player.txt");
        CHECK(resource.handle().valid());
        reference = resource.reference();
        assets.saveCatalog(catalog);
    }

    vshade::asset::AssetManager fresh;
    fresh.registerLoader<TextAsset>([](const std::filesystem::path& path) {
        return std::make_shared<TextAsset>(TextAsset{path.generic_string()});
    });
    fresh.loadCatalog(catalog);
    const auto resolved = fresh.loadResource(reference);
    REQUIRE(resolved);
    CHECK(resolved.handle() == reference.handle());
    CHECK(resolved->text == "assets/player.txt");
}

TEST_CASE("Asset manager validates loaders and resource types", "[asset]") {
    vshade::asset::AssetManager assets;
    CHECK_THROWS_AS(
        assets.load<TextAsset>("assets/player.txt"),
        std::invalid_argument
    );

    assets.registerLoader<TextAsset>([](const std::filesystem::path&) {
        return std::make_shared<TextAsset>(TextAsset{"loaded"});
    });
    const auto text = assets.load<TextAsset>("assets/shared.data");
    REQUIRE(text.valid());

    assets.registerLoader<OtherAsset>([](const std::filesystem::path&) {
        return std::make_shared<OtherAsset>();
    });
    CHECK_THROWS_AS(
        assets.load<OtherAsset>("assets/shared.data"),
        std::logic_error
    );

    assets.registerLoader<OtherAsset>([](const std::filesystem::path&) {
        return std::shared_ptr<OtherAsset>{};
    });
    CHECK_THROWS_AS(
        assets.load<OtherAsset>("assets/null.data"),
        std::runtime_error
    );

    assets.clear();
    CHECK(assets.size() == 0);
}

TEST_CASE("Asset manager owns registration for explicit stable handles", "[asset]") {
    vshade::asset::AssetManager assets;
    assets.registerLoader<TextAsset>([](const std::filesystem::path& path) {
        return std::make_shared<TextAsset>(TextAsset{path.generic_string()});
    });

    const auto handle = assets.registerAsset<TextAsset>(
        9001,
        "assets/dialogue/intro.txt"
    );
    CHECK(handle.id() == 9001);
    CHECK(assets.registeredAssetCount() == 1);
    CHECK_FALSE(assets.isLoaded(handle));

    assets.load(handle);
    REQUIRE(assets.get(handle));
    CHECK(assets.get(handle)->text == "assets/dialogue/intro.txt");

    CHECK(assets.unregisterAsset(handle));
    CHECK_FALSE(assets.isLoaded(handle));
    CHECK_FALSE(assets.metadata(handle).has_value());
    CHECK(assets.registeredAssetCount() == 0);
    CHECK_THROWS_AS(assets.load(handle), std::invalid_argument);
}

TEST_CASE("Asset registry catalogs stable IDs and normalized paths", "[asset]") {
    vshade::asset::AssetRegistry registry;
    registry.registerAsset({
        .id = 42,
        .sourcePath = "assets/characters/../player.png",
    });

    CHECK(registry.size() == 1);
    CHECK(registry.contains(42));
    CHECK(registry.contains("assets/player.png"));

    const auto original = registry.find(42);
    REQUIRE(original.has_value());
    CHECK(original->sourcePath == std::filesystem::path("assets/player.png"));
    CHECK(registry.find("assets/player.png") == original);

    registry.updatePath(42, "assets/textures/player.png");
    CHECK_FALSE(registry.contains("assets/player.png"));
    CHECK(registry.contains("assets/textures/player.png"));
    REQUIRE(registry.find(42).has_value());
    CHECK(registry.find(42)->id == 42);

    CHECK_THROWS_AS(
        registry.registerAsset({
            .id = 42,
            .sourcePath = "assets/duplicate-id.png",
        }),
        std::logic_error
    );
    CHECK_THROWS_AS(
        registry.registerAsset({
            .id = 99,
            .sourcePath = "assets/textures/player.png",
        }),
        std::logic_error
    );

    CHECK(registry.unregisterAsset(42));
    CHECK_FALSE(registry.unregisterAsset(42));
    CHECK(registry.size() == 0);
}

TEST_CASE("Asset catalog persists types and project-relative paths", "[asset]") {
    const std::filesystem::path root =
        std::filesystem::path(VSHADE_FILESYSTEM_OUTPUT_DIR) / "typed-root";
    const std::filesystem::path catalog = root / "AssetRegistry.json";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "assets" / "models");

    vshade::asset::AssetReference<vshade::renderer::Model> reference;
    {
        vshade::asset::AssetManager assets;
        assets.setRootDirectory(root);
        reference = assets.reference<vshade::renderer::Model>(
            root / "assets" / "models" / "robot.glb"
        );
        CHECK(
            reference.sourcePath()
            == std::filesystem::path("assets/models/robot.glb")
        );
        const auto metadata = assets.metadata(reference.handle());
        REQUIRE(metadata.has_value());
        CHECK(metadata->type == vshade::asset::AssetType::Model);
        assets.saveCatalog(catalog);
    }

    vshade::asset::AssetManager fresh;
    fresh.setRootDirectory(root);
    fresh.loadCatalog(catalog);
    const auto known = fresh.knownAssets<vshade::renderer::Model>();
    REQUIRE(known.size() == 1);
    CHECK(known[0].id == reference.handle().id());
    CHECK(known[0].type == vshade::asset::AssetType::Model);
    CHECK(known[0].sourcePath == std::filesystem::path("assets/models/robot.glb"));
    CHECK_FALSE(fresh.isLoaded(reference.handle()));
    CHECK(fresh.size() == 0);
}

TEST_CASE("Asset manager resolves catalog paths from the project root", "[asset]") {
    const std::filesystem::path root =
        std::filesystem::path(VSHADE_FILESYSTEM_OUTPUT_DIR) / "relative-root";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "assets");
    const std::filesystem::path source = root / "assets" / "note.txt";
    {
        std::ofstream output(source);
        REQUIRE(output);
        output << "hello";
    }

    vshade::asset::AssetManager assets;
    std::filesystem::path loadedPath;
    assets.registerLoader<TextAsset>([&loadedPath](const std::filesystem::path& path) {
        loadedPath = path;
        return std::make_shared<TextAsset>(TextAsset{path.generic_string()});
    });
    assets.setRootDirectory(root);

    const auto relative = assets.reference<TextAsset>("assets/note.txt");
    const auto absolute = assets.reference<TextAsset>(source);
    CHECK(relative.handle() == absolute.handle());
    CHECK(relative.sourcePath() == std::filesystem::path("assets/note.txt"));

    assets.load(relative.handle());
    REQUIRE(assets.isLoaded(relative.handle()));
    CHECK(std::filesystem::equivalent(loadedPath, source));
}

TEST_CASE("Asset catalog loads format 1 entries without types", "[asset]") {
    const std::filesystem::path catalog =
        std::filesystem::path(VSHADE_FILESYSTEM_OUTPUT_DIR) / "format-1-catalog.json";
    std::filesystem::create_directories(catalog.parent_path());
    {
        std::ofstream output(catalog);
        REQUIRE(output);
        output << R"({
  "FormatVersion": 1,
  "Assets": [
    {
      "Id": "42",
      "Path": "assets/models/robot.glb"
    }
  ]
}
)";
    }

    vshade::asset::AssetRegistry registry;
    registry.load(catalog);
    REQUIRE(registry.size() == 1);
    const auto metadata = registry.find(42);
    REQUIRE(metadata.has_value());
    CHECK(metadata->sourcePath == std::filesystem::path("assets/models/robot.glb"));
    CHECK(metadata->type == vshade::asset::AssetType::Unknown);

    vshade::asset::AssetManager assets;
    assets.loadCatalog(catalog);
    CHECK(assets.registeredAssetCount() == 1);
    CHECK(assets.knownAssets<vshade::renderer::Model>().empty());
}

TEST_CASE("Default scene loader deserializes a scene asset", "[asset]") {
    vshade::asset::AssetManager assets;

    const auto handle = assets.load<vshade::scene::Scene>(
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "scene" / "scene_example.json"
    );
    const std::shared_ptr<vshade::scene::Scene> scene = assets.get(handle);

    REQUIRE(scene);
    CHECK(scene->name() == "Example");
    CHECK(assets.registeredAssetCount() == 1);
}

TEST_CASE("Default prefab loader creates reusable scene templates", "[asset][prefab]") {
    vshade::asset::AssetManager assets;
    const auto prefab = assets.loadResource<vshade::scene::Prefab>(
        std::filesystem::path(VSHADE_GOLDEN_DIR) / "scene" / "scene_example.json"
    );
    REQUIRE(prefab);
    CHECK(prefab->scene().name() == "Example");
    vshade::scene::Scene destination("Prefab destination");
    CHECK(destination.instantiate(*prefab));
}

TEST_CASE("Default renderer loaders create texture shader and mesh resources", "[asset][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::asset::AssetManager assets;
    vshade::asset::AssetManager modelAssets;

    const auto textureHandle = assets.load<vshade::renderer::Texture2D>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "player.png"
    );
    const auto shaderHandle = assets.load<vshade::renderer::Shader>(
        std::filesystem::path(VSHADE_SANDBOX_SHADER_DIR) / "shader.vs"
    );
    const auto meshHandle = assets.load<vshade::renderer::Mesh>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "triangle.gltf"
    );
    const auto modelHandle = modelAssets.load<vshade::renderer::Model>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "triangle.gltf"
    );

    const auto texture = assets.get(textureHandle);
    const auto shader = assets.get(shaderHandle);
    const auto mesh = assets.get(meshHandle);
    const auto model = modelAssets.get(modelHandle);
    REQUIRE(texture);
    REQUIRE(shader);
    REQUIRE(mesh);
    REQUIRE(model);
    CHECK(texture->width() > 0);
    CHECK(texture->height() > 0);
    CHECK(shader->name() == "shader");
    REQUIRE(mesh->vertexArray());
    REQUIRE(mesh->vertexArray()->indexBuffer());
    CHECK(mesh->vertexArray()->vertexBuffers().size() == 1);
    CHECK(mesh->vertexArray()->indexBuffer()->count() == 3);
    CHECK(mesh->topology() == vshade::renderer::PrimitiveTopology::Triangles);
    REQUIRE(model->primitives().size() == 2);
    CHECK(model->primitives()[0].mesh->topology() ==
          vshade::renderer::PrimitiveTopology::Triangles);
    CHECK(model->primitives()[1].mesh->topology() ==
          vshade::renderer::PrimitiveTopology::Points);
    REQUIRE(model->nodes().size() == 1);
    CHECK(model->nodes()[0].name == "TriangleNode");
    CHECK(model->nodes()[0].primitives == std::vector<std::size_t>{0, 1});
    CHECK(model->nodes()[0].localTransform.position() ==
          vshade::math::Vec3{1.0F, 2.0F, 3.0F});
    CHECK(model->nodes()[0].localTransform.scale() ==
          vshade::math::Vec3{2.0F, 2.0F, 2.0F});
    REQUIRE(model->rootNodes().size() == 1);
    CHECK(model->rootNodes()[0] == 0);
    REQUIRE(model->primitives()[0].material);
    CHECK(model->primitives()[0].material->albedoColor() ==
          vshade::math::Vec4{0.25F, 0.5F, 0.75F, 1.0F});
    CHECK(model->primitives()[0].material->roughness() == Catch::Approx(0.4F));
    CHECK(model->primitives()[0].material->metallic() == Catch::Approx(0.2F));
    CHECK(assets.size() == 3);
    CHECK(modelAssets.size() == 1);
}

TEST_CASE("Default model loader imports the sandbox Damaged Helmet", "[asset][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::asset::AssetManager assets;

    const auto handle = assets.load<vshade::renderer::Model>(
        std::filesystem::path(VSHADE_SANDBOX_ASSET_DIR) / "DamagedHelmet.glb"
    );
    const std::shared_ptr<vshade::renderer::Model> model = assets.get(handle);

    REQUIRE(model);
    CHECK_FALSE(model->primitives().empty());
    CHECK_FALSE(model->nodes().empty());
    CHECK_FALSE(model->rootNodes().empty());
    bool foundBaseColorTexture = false;
    for (const vshade::renderer::ModelPrimitive& primitive : model->primitives()) {
        REQUIRE(primitive.mesh);
        REQUIRE(primitive.material);
        REQUIRE(primitive.mesh->vertexArray());
        REQUIRE(primitive.mesh->vertexArray()->indexBuffer());
        CHECK(primitive.mesh->vertexArray()->indexBuffer()->count() > 0);
        if (primitive.material->hasAlbedoTexture()) {
            foundBaseColorTexture = true;
            CHECK(primitive.material->albedoTexture()->width() > 0);
            CHECK(primitive.material->albedoTexture()->height() > 0);
        }
    }
    CHECK(foundBaseColorTexture);
}

TEST_CASE("Model loader imports eye alpha and normal-map materials", "[asset][opengl]") {
    vshade::tests::visual::HiddenRenderContext context(64, 64);
    vshade::asset::AssetManager assets;

    const auto handle = assets.load<vshade::renderer::Model>(
        std::filesystem::path(VSHADE_TEST_ASSET_DIR) / "eye.glb"
    );
    const std::shared_ptr<vshade::renderer::Model> model = assets.get(handle);

    REQUIRE(model);
    REQUIRE(model->primitives().size() == 2);
    const auto& glass = model->primitives()[0].material;
    const auto& eye = model->primitives()[1].material;
    REQUIRE(glass);
    REQUIRE(eye);
    CHECK(glass->alphaMode() == vshade::renderer::MaterialAlphaMode::Blend);
    CHECK(glass->doubleSided());
    CHECK(glass->albedoColor().a == Catch::Approx(0.1085521F));
    CHECK_FALSE(glass->hasNormalTexture());
    CHECK(eye->alphaMode() == vshade::renderer::MaterialAlphaMode::Opaque);
    CHECK(eye->doubleSided());
    CHECK(eye->hasAlbedoTexture());
    CHECK(eye->hasNormalTexture());
    CHECK(eye->normalScale() == Catch::Approx(1.0F));
    REQUIRE(eye->normalTexture());
    CHECK(eye->normalTexture()->width() > 0);
    CHECK(eye->normalTexture()->height() > 0);
}
