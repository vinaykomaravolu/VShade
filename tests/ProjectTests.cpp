#include <catch2/catch_test_macros.hpp>

#include <project/Project.hpp>
#include <project/ProjectSerializer.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace {

[[nodiscard]] std::filesystem::path projectOutput(
    const char* directoryName
) {
    return std::filesystem::path(VSHADE_PROJECT_OUTPUT_DIR)
        / directoryName;
}

} // namespace

TEST_CASE("Projects create and load portable configuration", "[project]") {
    const std::filesystem::path directory =
        projectOutput("create-and-load");
    std::filesystem::remove_all(directory);

    const auto created = vshade::project::Project::create(directory);
    REQUIRE(created);
    CHECK(created->config().name == "create-and-load");
    CHECK(created->config().assetDirectory == "assets");
    CHECK(created->config().startScene.empty());
    CHECK(std::filesystem::is_regular_file(created->projectFile()));
    CHECK(std::filesystem::is_directory(created->assetDirectory()));
    CHECK(std::filesystem::is_directory(created->assetDirectory() / "models"));
    CHECK(std::filesystem::is_directory(created->assetDirectory() / "textures"));
    CHECK(std::filesystem::is_directory(created->assetDirectory() / "audio"));
    CHECK(std::filesystem::is_directory(created->sceneDirectory()));
    CHECK(created->assetRegistryPath()
        == created->projectDirectory() / "AssetRegistry.json");

    nlohmann::json json;
    {
        std::ifstream input(created->projectFile());
        REQUIRE(input);
        input >> json;
    }
    CHECK(json.at("format").get<int>() == 1);
    CHECK(json.at("name").get<std::string>() == "create-and-load");
    CHECK(json.at("assetDirectory").get<std::string>() == "assets");
    CHECK(json.at("startScene").get<std::string>().empty());

    const auto loaded =
        vshade::project::Project::load(created->projectFile());
    REQUIRE(loaded);
    CHECK(loaded->config().name == created->config().name);
    CHECK(loaded->assetDirectory() == created->assetDirectory());
    CHECK(loaded->sceneDirectory() == created->sceneDirectory());
}

TEST_CASE("Project serializer preserves a startup scene", "[project]") {
    const std::filesystem::path directory =
        projectOutput("startup-scene");
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    const std::filesystem::path projectFile =
        directory / "Game.vshade";

    vshade::project::ProjectConfig config;
    config.name = "Game";
    config.assetDirectory = "content";
    config.startScene = "content/scenes/Main.vscene";
    REQUIRE(vshade::project::ProjectSerializer::serialize(
        config,
        projectFile
    ));

    const auto project = vshade::project::Project::load(projectFile);
    REQUIRE(project);
    CHECK(project->config().startScene == config.startScene);
    CHECK(
        project->startScenePath()
        == directory / config.startScene
    );
    CHECK(std::filesystem::is_directory(directory / "content"));
    CHECK(std::filesystem::is_directory(directory / "content" / "scenes"));
}

TEST_CASE("Projects persist a recorded start scene", "[project]") {
    const std::filesystem::path directory =
        projectOutput("record-start-scene");
    std::filesystem::remove_all(directory);

    const auto project = vshade::project::Project::create(directory);
    REQUIRE(project);
    const std::filesystem::path scenePath =
        project->sceneDirectory() / "Main.vscene";
    project->setStartScene(scenePath);
    REQUIRE(project->save());
    CHECK(
        project->config().startScene
        == std::filesystem::path("assets/scenes/Main.vscene")
    );

    const auto loaded = vshade::project::Project::load(project->projectFile());
    REQUIRE(loaded);
    CHECK(loaded->config().startScene == project->config().startScene);
    CHECK(loaded->startScenePath() == scenePath);
}

TEST_CASE("Project serializer rejects paths outside the project", "[project]") {
    const std::filesystem::path directory =
        projectOutput("invalid-path");
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    vshade::project::ProjectConfig config;
    config.assetDirectory = "../outside";
    const auto result = vshade::project::ProjectSerializer::serialize(
        config,
        directory / "Invalid.vshade"
    );
    CHECK_FALSE(result);
}
