#pragma once

#include "core/Result.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace vshade::project {

struct ProjectConfig {
    std::string name = "Untitled";
    std::filesystem::path assetDirectory = "assets";
    std::filesystem::path startScene;
};

/** @brief Owns the portable configuration and resolved paths of a VShade project. */
class Project final {
public:
    [[nodiscard]] static std::shared_ptr<Project> create(
        const std::filesystem::path& directory
    );

    [[nodiscard]] static std::shared_ptr<Project> load(
        const std::filesystem::path& projectFile
    );

    [[nodiscard]] const ProjectConfig& config() const noexcept;
    [[nodiscard]] const std::filesystem::path& projectFile() const noexcept;
    [[nodiscard]] std::filesystem::path projectDirectory() const;
    [[nodiscard]] std::filesystem::path assetDirectory() const;
    [[nodiscard]] std::filesystem::path sceneDirectory() const;
    [[nodiscard]] std::filesystem::path startScenePath() const;
    [[nodiscard]] std::filesystem::path assetRegistryPath() const;

    void setStartScene(const std::filesystem::path& path);
    [[nodiscard]] core::Result<void> save() const;

private:
    Project(
        ProjectConfig config,
        std::filesystem::path projectFile
    );

    [[nodiscard]] static std::shared_ptr<Project> make(
        ProjectConfig config,
        std::filesystem::path projectFile
    );

    void createDirectories() const;

    ProjectConfig m_config;
    std::filesystem::path m_projectFile;
};

} // namespace vshade::project
