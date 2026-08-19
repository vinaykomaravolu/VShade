#include "Project.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace editor {

Project::Project(
    std::filesystem::path directory,
    std::filesystem::path projectFile
) : m_directory(std::move(directory)),
    m_projectFile(std::move(projectFile)) {}

std::shared_ptr<Project> Project::create(
    const std::filesystem::path& directory
) {
    if (directory.empty()) {
        throw std::invalid_argument("A project directory cannot be empty");
    }

    const std::filesystem::path normalizedDirectory =
        std::filesystem::absolute(directory).lexically_normal();
    const std::string projectName =
        normalizedDirectory.filename().generic_string();
    if (projectName.empty() || projectName == "." || projectName == "..") {
        throw std::invalid_argument("The project directory must have a name");
    }

    std::filesystem::create_directories(normalizedDirectory);
    const std::filesystem::path projectFile =
        normalizedDirectory / (projectName + ".vshade");
    if (std::filesystem::exists(projectFile)) {
        throw std::logic_error("A VShade project already exists in this directory");
    }

    auto project = std::shared_ptr<Project>(
        new Project(normalizedDirectory, projectFile)
    );
    project->createDirectories();

    std::ofstream output(projectFile, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Unable to create the VShade project file");
    }
    output << "{\n  \"format\": 1\n}\n";
    if (!output) {
        throw std::runtime_error("Unable to write the VShade project file");
    }
    return project;
}

std::shared_ptr<Project> Project::open(
    const std::filesystem::path& projectFile
) {
    if (projectFile.empty()
        || projectFile.extension() != ".vshade"
        || !std::filesystem::is_regular_file(projectFile)) {
        throw std::invalid_argument("A valid .vshade project file is required");
    }

    const std::filesystem::path normalizedFile =
        std::filesystem::absolute(projectFile).lexically_normal();
    auto project = std::shared_ptr<Project>(
        new Project(normalizedFile.parent_path(), normalizedFile)
    );
    project->createDirectories();
    return project;
}

const std::filesystem::path& Project::directory() const noexcept {
    return m_directory;
}

const std::filesystem::path& Project::projectFile() const noexcept {
    return m_projectFile;
}

std::filesystem::path Project::assetDirectory() const {
    return m_directory / "assets";
}

void Project::createDirectories() const {
    std::filesystem::create_directories(assetDirectory());
}

} // namespace editor
