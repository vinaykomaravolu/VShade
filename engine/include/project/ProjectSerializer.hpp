#pragma once

#include "core/Result.hpp"
#include "project/Project.hpp"

#include <filesystem>

namespace vshade::project {

/** @brief Reads and writes the portable JSON contents of a .vshade file. */
class ProjectSerializer final {
public:
    [[nodiscard]] static core::Result<void> serialize(
        const ProjectConfig& config,
        const std::filesystem::path& projectFile
    );

    [[nodiscard]] static core::Result<ProjectConfig> deserialize(
        const std::filesystem::path& projectFile
    );
};

} // namespace vshade::project
