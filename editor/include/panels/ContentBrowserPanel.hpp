#pragma once

#include <filesystem>
#include <optional>
#include <system_error>
#include <vector>

namespace editor {

/** @brief Filesystem-backed view of the active project's Assets directory. */
class ContentBrowserPanel final {
public:
    explicit ContentBrowserPanel(std::filesystem::path rootDirectory);

    void setRoot(std::filesystem::path rootDirectory);

    /** @brief Navigates to @p path and selects it when it is under the current root. */
    void reveal(const std::filesystem::path& path);
    void refresh() noexcept;

    /** @brief Draws the panel and returns a scene path when one is opened. */
    [[nodiscard]] std::optional<std::filesystem::path> onImGuiRender();

private:
    void refreshEntries();

    std::filesystem::path m_rootDirectory;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_selectedPath;
    std::vector<std::filesystem::directory_entry> m_entries;
    std::error_code m_directoryError;
    bool m_focusRequested = false;
    bool m_refreshRequested = true;
};

} // namespace editor
