#pragma once

#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vshade::asset { class AssetManager; }
namespace vshade::renderer { class Texture2D; }

namespace editor {

/** @brief Filesystem-backed view of the active project's Assets directory. */
class ContentBrowserPanel final {
public:
    explicit ContentBrowserPanel(std::filesystem::path rootDirectory);

    void setRoot(std::filesystem::path rootDirectory);
    void setAssetManager(vshade::asset::AssetManager& assets) noexcept;
    void setOperationHandler(
        std::function<void(std::string message, bool success)> handler
    );

    /** @brief Navigates to @p path and selects it when it is under the current root. */
    void reveal(const std::filesystem::path& path);
    void refresh() noexcept;

    /** @brief Draws the panel and returns a scene path when one is opened. */
    [[nodiscard]] std::optional<std::filesystem::path> onImGuiRender();

private:
    void refreshEntries();
    void navigateTo(std::filesystem::path directory, bool recordHistory = true);
    void drawBreadcrumbs();

    std::filesystem::path m_rootDirectory;
    std::filesystem::path m_currentDirectory;
    std::filesystem::path m_selectedPath;
    std::vector<std::filesystem::directory_entry> m_entries;
    std::vector<std::filesystem::path> m_backHistory;
    std::vector<std::filesystem::path> m_forwardHistory;
    std::vector<std::filesystem::path> m_favorites;
    vshade::asset::AssetManager* m_assets = nullptr;
    std::function<void(std::string, bool)> m_operationHandler;
    std::unordered_map<std::string, std::shared_ptr<vshade::renderer::Texture2D>>
        m_textureThumbnails;
    std::unordered_set<std::string> m_importFailures;
    std::error_code m_directoryError;
    std::array<char, 160> m_search{};
    std::array<char, 256> m_renameBuffer{};
    std::filesystem::path m_pendingRename;
    std::filesystem::path m_pendingDelete;
    std::filesystem::file_time_type m_directoryWriteTime{};
    std::chrono::steady_clock::time_point m_nextFilesystemPoll{};
    float m_thumbnailSize = 88.0F;
    bool m_listView = false;
    bool m_openRename = false;
    bool m_openDelete = false;
    bool m_focusRequested = false;
    bool m_refreshRequested = true;
};

} // namespace editor
