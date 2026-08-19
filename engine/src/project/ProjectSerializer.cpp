#include "project/ProjectSerializer.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace vshade::project {
namespace {

using Json = nlohmann::json;

[[nodiscard]] core::Diagnostic diagnostic(
    std::string code,
    std::string message,
    const std::filesystem::path& path
) {
    return {
        core::DiagnosticSeverity::Error,
        std::move(code),
        std::move(message),
        path,
    };
}

[[nodiscard]] std::filesystem::path portablePath(
    const std::filesystem::path& value,
    const char* field,
    const bool allowEmpty
) {
    if (value.empty() && allowEmpty) {
        return {};
    }

    const std::filesystem::path path =
        value.lexically_normal();
    if (path.empty()
        || path.is_absolute()
        || *path.begin() == "..") {
        throw std::invalid_argument(
            std::string(field) + " must be a project-relative path"
        );
    }
    return path;
}

[[nodiscard]] std::filesystem::path portablePath(
    const Json& json,
    const char* field,
    const bool allowEmpty
) {
    return portablePath(
        std::filesystem::path(json.at(field).get<std::string>()),
        field,
        allowEmpty
    );
}

} // namespace

core::Result<void> ProjectSerializer::serialize(
    const ProjectConfig& config,
    const std::filesystem::path& projectFile
) {
    try {
        if (config.name.empty()) {
            throw std::invalid_argument("project name cannot be empty");
        }
        const std::filesystem::path assetDirectory =
            portablePath(config.assetDirectory, "assetDirectory", false);
        const std::filesystem::path startScene =
            portablePath(config.startScene, "startScene", true);

        const Json json{
            {"format", 1},
            {"name", config.name},
            {"assetDirectory", assetDirectory.generic_string()},
            {"startScene", startScene.generic_string()},
        };

        std::ofstream output(
            projectFile,
            std::ios::binary | std::ios::trunc
        );
        if (!output) {
            throw std::runtime_error("unable to open project file for writing");
        }
        output << json.dump(4) << '\n';
        if (!output) {
            throw std::runtime_error("unable to write project file");
        }
        return core::Result<void>::success();
    } catch (const std::exception& error) {
        return core::Result<void>::failure(diagnostic(
            "project.serialize",
            error.what(),
            projectFile
        ));
    }
}

core::Result<ProjectConfig> ProjectSerializer::deserialize(
    const std::filesystem::path& projectFile
) {
    try {
        std::ifstream input(projectFile, std::ios::binary);
        if (!input) {
            throw std::runtime_error("unable to open project file");
        }

        const Json json = Json::parse(input);
        if (json.at("format").get<int>() != 1) {
            throw std::invalid_argument("unsupported project format");
        }

        ProjectConfig config;
        config.name = json.value(
            "name",
            projectFile.stem().generic_string()
        );
        if (config.name.empty()) {
            throw std::invalid_argument("project name cannot be empty");
        }
        config.assetDirectory = json.contains("assetDirectory")
            ? portablePath(json, "assetDirectory", false)
            : std::filesystem::path("assets");
        config.startScene = json.contains("startScene")
            ? portablePath(json, "startScene", true)
            : std::filesystem::path{};
        return core::Result<ProjectConfig>::success(std::move(config));
    } catch (const std::exception& error) {
        return core::Result<ProjectConfig>::failure(diagnostic(
            "project.deserialize",
            error.what(),
            projectFile
        ));
    }
}

} // namespace vshade::project
