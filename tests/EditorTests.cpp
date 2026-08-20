#include <catch2/catch_test_macros.hpp>

#include <AssetImportService.hpp>
#include <EditorDocument.hpp>
#include <UndoHistory.hpp>

#include <project/Project.hpp>
#include <scene/Scene.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

[[nodiscard]] std::filesystem::path editorOutput(const char* name) {
    return std::filesystem::path(VSHADE_EDITOR_OUTPUT_DIR) / name;
}

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output);
    output << text;
}

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    REQUIRE(input);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };
}

} // namespace

TEST_CASE("Editor documents track saved revisions independently of history depth", "[editor]") {
    editor::EditorDocument document;
    document.reset("Scenes/Main.vscene", 4);
    CHECK_FALSE(document.isDirty());
    CHECK(document.savedRevision() == 4);

    document.setCurrentRevision(5);
    CHECK(document.isDirty());
    document.setCurrentRevision(4);
    CHECK_FALSE(document.isDirty());

    document.setCurrentRevision(9);
    document.markSaved();
    CHECK_FALSE(document.isDirty());
    CHECK(document.displayName() == "Main.vscene");
}

TEST_CASE("Editor undo history restores document revisions", "[editor]") {
    vshade::scene::Scene scene;
    vshade::scene::Entity selected;
    editor::UndoHistory history;
    editor::EditorDocument document;
    document.reset();

    history.begin(scene, selected);
    selected = scene.create("Player");
    REQUIRE(history.commit(scene, selected));
    REQUIRE(history.currentRevision() == 1);
    document.setCurrentRevision(history.currentRevision());
    document.markSaved();

    history.begin(scene, selected);
    selected.setName("Hero");
    REQUIRE(history.commit(scene, selected));
    document.setCurrentRevision(history.currentRevision());
    CHECK(document.isDirty());

    REQUIRE(history.undo(scene, selected));
    document.setCurrentRevision(history.currentRevision());
    CHECK(std::string(selected.name()) == "Player");
    CHECK_FALSE(document.isDirty());

    REQUIRE(history.redo(scene, selected));
    document.setCurrentRevision(history.currentRevision());
    CHECK(std::string(selected.name()) == "Hero");
    CHECK(document.isDirty());
}

TEST_CASE("Asset imports stay in the project and honor collision policy", "[editor]") {
    const std::filesystem::path root = editorOutput("asset-import");
    std::filesystem::remove_all(root);
    const auto project = vshade::project::Project::create(root / "Project");
    const std::filesystem::path source = root / "external" / "icon.png";
    writeText(source, "first");

    const editor::AssetImportResult first = editor::AssetImportService::import(
        *project,
        source,
        editor::AssetCollisionPolicy::Cancel
    );
    REQUIRE(first);
    CHECK(first.destination.parent_path() == project->assetDirectory() / "textures");
    CHECK(readText(first.destination) == "first");

    writeText(source, "second");
    const editor::AssetImportResult cancelled = editor::AssetImportService::import(
        *project,
        source,
        editor::AssetCollisionPolicy::Cancel
    );
    CHECK_FALSE(cancelled);
    CHECK(cancelled.cancelled);

    const editor::AssetImportResult kept = editor::AssetImportService::import(
        *project,
        source,
        editor::AssetCollisionPolicy::KeepBoth
    );
    REQUIRE(kept);
    CHECK(kept.destination != first.destination);
    CHECK(readText(kept.destination) == "second");

    const editor::AssetImportResult replaced = editor::AssetImportService::import(
        *project,
        source,
        editor::AssetCollisionPolicy::Replace
    );
    REQUIRE(replaced);
    CHECK(replaced.destination == first.destination);
    CHECK(readText(replaced.destination) == "second");
}
