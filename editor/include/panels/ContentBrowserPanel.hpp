#pragma once

#include <filesystem>
#include <optional>

namespace editor {

/** @brief Filesystem-backed view of the active project's Assets directory. */
class ContentBrowserPanel final {
public:
    explicit ContentBrowserPanel(std::filesystem::path rootDirectory);

    void setRoot(std::filesystem::path rootDirectory);

    /** @brief Draws the panel and returns a scene path when one is opened. */
    [[nodiscard]] std::optional<std::filesystem::path> onImGuiRender();

private:
    std::filesystem::path m_rootDirectory;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_selectedPath;
};

} // namespace editor
