#pragma once

#include <filesystem>
#include <memory>

namespace editor {

/** @brief Owns the editor-level paths associated with one VShade project. */
class Project final {
public:
    [[nodiscard]] static std::shared_ptr<Project> create(
        const std::filesystem::path& directory
    );

    [[nodiscard]] static std::shared_ptr<Project> open(
        const std::filesystem::path& projectFile
    );

    [[nodiscard]] const std::filesystem::path& directory() const noexcept;
    [[nodiscard]] const std::filesystem::path& projectFile() const noexcept;
    [[nodiscard]] std::filesystem::path assetDirectory() const;

private:
    Project(
        std::filesystem::path directory,
        std::filesystem::path projectFile
    );

    void createDirectories() const;

    std::filesystem::path m_directory;
    std::filesystem::path m_projectFile;
};

} // namespace editor
