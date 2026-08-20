#pragma once

#include <filesystem>
#include <optional>

namespace editor {

/** @brief Filesystem-backed view of the active project's Assets directory. */
class ContentBrowserPanel final {
public:
    explicit ContentBrowserPanel(std::filesystem::path rootDirectory);

    void setRoot(std::filesystem::path rootDirectory);

    /** @brief Navigates to @p path and selects it when it is under the current root. */
    void reveal(const std::filesystem::path& path);

    /** @brief Draws the panel and returns a scene path when one is opened. */
    [[nodiscard]] std::optional<std::filesystem::path> onImGuiRender();

private:
    std::filesystem::path m_rootDirectory;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_selectedPath;
    bool m_focusRequested = false;
};

} // namespace editor
