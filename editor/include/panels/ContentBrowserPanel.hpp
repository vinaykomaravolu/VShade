#pragma once

#include <filesystem>
#include <optional>

namespace editor {

/** @brief Filesystem-backed view of the active project's Assets directory. */
class ContentBrowserPanel final {
public:
    explicit ContentBrowserPanel(std::filesystem::path assetDirectory);

    void setAssetDirectory(std::filesystem::path assetDirectory);

    /** @brief Draws the panel and returns a scene path when one is opened. */
    [[nodiscard]] std::optional<std::filesystem::path> onImGuiRender();

private:
    std::filesystem::path m_assetDirectory;
    std::filesystem::path m_currentDirectory;
};

} // namespace editor
