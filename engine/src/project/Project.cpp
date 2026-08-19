#include "project/Project.hpp"

#include "project/ProjectSerializer.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::project {

Project::Project(
    ProjectConfig config,
    std::filesystem::path projectFile
) : m_config(std::move(config)),
    m_projectFile(std::move(projectFile)) {}

std::shared_ptr<Project> Project::create(
    const std::filesystem::path& directory
) {
    if (directory.empty()) {
        throw std::invalid_argument("a project directory cannot be empty");
    }

    const std::filesystem::path normalizedDirectory =
        std::filesystem::absolute(directory).lexically_normal();
    const std::string projectName =
        normalizedDirectory.filename().generic_string();
    if (projectName.empty() || projectName == "." || projectName == "..") {
        throw std::invalid_argument("the project directory must have a name");
    }

    std::filesystem::create_directories(normalizedDirectory);
    const std::filesystem::path projectFile =
        normalizedDirectory / (projectName + ".vshade");
    if (std::filesystem::exists(projectFile)) {
        throw std::logic_error(
            "a VShade project already exists in this directory"
        );
    }

    ProjectConfig config;
    config.name = projectName;
    auto project = make(std::move(config), projectFile);
    project->createDirectories();

    const auto result = project->save();
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return project;
}

std::shared_ptr<Project> Project::load(
    const std::filesystem::path& projectFile
) {
    if (projectFile.empty()
        || projectFile.extension() != ".vshade"
        || !std::filesystem::is_regular_file(projectFile)) {
        throw std::invalid_argument(
            "a valid .vshade project file is required"
        );
    }

    const std::filesystem::path normalizedFile =
        std::filesystem::absolute(projectFile).lexically_normal();
    auto result = ProjectSerializer::deserialize(normalizedFile);
    if (!result) {
        throw std::runtime_error(result.error().message);
    }

    auto project = make(std::move(result.value()), normalizedFile);
    project->createDirectories();
    return project;
}

std::shared_ptr<Project> Project::make(
    ProjectConfig config,
    std::filesystem::path projectFile
) {
    return std::shared_ptr<Project>(
        new Project(std::move(config), std::move(projectFile))
    );
}

const ProjectConfig& Project::config() const noexcept {
    return m_config;
}

const std::filesystem::path& Project::projectFile() const noexcept {
    return m_projectFile;
}

std::filesystem::path Project::projectDirectory() const {
    return m_projectFile.parent_path();
}

std::filesystem::path Project::assetDirectory() const {
    return projectDirectory() / m_config.assetDirectory;
}

std::filesystem::path Project::sceneDirectory() const {
    return assetDirectory() / "scenes";
}

std::filesystem::path Project::prefabDirectory() const {
    return assetDirectory() / "prefabs";
}

std::filesystem::path Project::assetRegistryPath() const {
    return projectDirectory() / "AssetRegistry.json";
}

std::filesystem::path Project::startScenePath() const {
    if (m_config.startScene.empty()) {
        return {};
    }
    return projectDirectory() / m_config.startScene;
}

void Project::setStartScene(const std::filesystem::path& path) {
    if (path.empty()) {
        m_config.startScene.clear();
        return;
    }

    std::filesystem::path relative = path;
    if (relative.is_absolute()) {
        relative = relative.lexically_relative(projectDirectory());
    }
    relative = relative.lexically_normal();
    if (relative.empty()
        || relative.is_absolute()
        || *relative.begin() == "..") {
        throw std::invalid_argument(
            "startScene must be a project-relative path"
        );
    }
    m_config.startScene = std::move(relative);
}

core::Result<void> Project::save() const {
    return ProjectSerializer::serialize(m_config, m_projectFile);
}

void Project::createDirectories() const {
    std::filesystem::create_directories(assetDirectory());
    std::filesystem::create_directories(assetDirectory() / "models");
    std::filesystem::create_directories(assetDirectory() / "textures");
    std::filesystem::create_directories(assetDirectory() / "audio");
    std::filesystem::create_directories(prefabDirectory());
    std::filesystem::create_directories(sceneDirectory());
}

} // namespace vshade::project
