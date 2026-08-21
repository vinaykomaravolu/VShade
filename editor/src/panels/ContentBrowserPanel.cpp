#include "panels/ContentBrowserPanel.hpp"
#include "EditorIcons.hpp"
#include "ImGui/ImGuiTheme.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <renderer/Model.hpp>
#include <renderer/Texture.hpp>
#include <scene/Prefab.hpp>
#include <scene/Scene.hpp>
#include <scene/SceneSerializer.hpp>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <imgui.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif

namespace editor {
namespace {

constexpr float cardPadding = 8.0F;
constexpr float labelHeight = 22.0F;
constexpr float cardRounding = 8.0F;

[[nodiscard]] std::string lowercase(std::string value) {
    std::ranges::transform(
        value,
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );
    return value;
}

[[nodiscard]] const char* typeName(
    const bool directory,
    const vshade::asset::AssetType type
) {
    if (directory) {
        return "Folder";
    }
    switch (type) {
        case vshade::asset::AssetType::Texture:
            return "Texture";
        case vshade::asset::AssetType::Model:
            return "Model";
        case vshade::asset::AssetType::Audio:
            return "Audio Clip";
        case vshade::asset::AssetType::Scene:
            return "Scene";
        case vshade::asset::AssetType::Prefab:
            return "Prefab";
        case vshade::asset::AssetType::Shader:
            return "Shader";
        default:
            return "File";
    }
}

[[nodiscard]] ImU32 typeColor(
    const bool directory,
    const vshade::asset::AssetType type
) {
    if (directory) {
        return IM_COL32(232, 191, 74, 255);
    }
    switch (type) {
        case vshade::asset::AssetType::Texture:
            return IM_COL32(91, 141, 239, 255);
        case vshade::asset::AssetType::Model:
            return IM_COL32(107, 203, 119, 255);
        case vshade::asset::AssetType::Audio:
            return IM_COL32(181, 123, 255, 255);
        case vshade::asset::AssetType::Scene:
            return IM_COL32(78, 205, 196, 255);
        case vshade::asset::AssetType::Prefab:
            return IM_COL32(240, 160, 90, 255);
        default:
            return IM_COL32(138, 143, 152, 255);
    }
}

[[nodiscard]] std::shared_ptr<vshade::renderer::Texture2D> iconForEntry(
    const bool directory,
    const vshade::asset::AssetType type
) {
    if (directory) {
        return EditorIcons::folder();
    }
    return EditorIcons::forAsset(type);
}

[[nodiscard]] std::string ellipsize(const std::string& text, const float maxWidth) {
    if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) {
        return text;
    }

    std::string clipped = text;
    while (!clipped.empty()
        && ImGui::CalcTextSize((clipped + "...").c_str()).x > maxWidth) {
        clipped.pop_back();
    }
    return clipped + "...";
}

[[nodiscard]] std::string currentLocationLabel(
    const std::filesystem::path& root,
    const std::filesystem::path& current
) {
    if (root.empty()) {
        return {};
    }
    if (current == root) {
        return root.filename().generic_string().empty()
            ? "Assets"
            : root.filename().generic_string();
    }

    std::error_code error;
    const std::filesystem::path relative =
        std::filesystem::relative(current, root, error);
    if (error || relative.empty() || relative.generic_string() == ".") {
        return current.generic_string();
    }
    return relative.generic_string();
}

} // namespace

ContentBrowserPanel::ContentBrowserPanel(
    std::filesystem::path rootDirectory
) {
    setRoot(std::move(rootDirectory));
}

void ContentBrowserPanel::setRoot(
    std::filesystem::path rootDirectory
) {
    m_rootDirectory = std::move(rootDirectory).lexically_normal();
    m_currentDirectory = m_rootDirectory;
    m_selectedPath.clear();
    m_backHistory.clear();
    m_forwardHistory.clear();
    m_favorites.clear();
    m_textureThumbnails.clear();
    m_importFailures.clear();
    refresh();
}

void ContentBrowserPanel::setAssetManager(
    vshade::asset::AssetManager& assets
) noexcept {
    m_assets = &assets;
}

void ContentBrowserPanel::setOperationHandler(
    std::function<void(std::string message, bool success)> handler
) {
    m_operationHandler = std::move(handler);
}

void ContentBrowserPanel::setSelectionHandler(
    std::function<void(std::filesystem::path)> handler
) {
    m_selectionHandler = std::move(handler);
}

void ContentBrowserPanel::navigateTo(
    std::filesystem::path directory,
    const bool recordHistory
) {
    directory = std::move(directory).lexically_normal();
    std::error_code error;
    const auto relative = std::filesystem::relative(
        directory,
        m_rootDirectory,
        error
    );
    if (error || relative.generic_string().starts_with("..")
        || !std::filesystem::is_directory(directory, error)) {
        return;
    }
    if (recordHistory && directory != m_currentDirectory) {
        m_backHistory.push_back(m_currentDirectory);
        m_forwardHistory.clear();
    }
    m_currentDirectory = std::move(directory);
    m_selectedPath.clear();
    refresh();
}

void ContentBrowserPanel::drawBreadcrumbs() {
    std::vector<std::filesystem::path> crumbs{m_rootDirectory};
    std::error_code error;
    const auto relative = std::filesystem::relative(
        m_currentDirectory,
        m_rootDirectory,
        error
    );
    if (!error && relative != ".") {
        auto cursor = m_rootDirectory;
        for (const auto& part : relative) {
            cursor /= part;
            crumbs.push_back(cursor);
        }
    }
    for (std::size_t index = 0; index < crumbs.size(); ++index) {
        if (index != 0) {
            ImGui::SameLine(0.0F, ui::scaled(4.0F));
            ImGui::TextDisabled(">");
            ImGui::SameLine(0.0F, ui::scaled(4.0F));
        }
        const std::string label = index == 0
            ? "Assets"
            : crumbs[index].filename().generic_string();
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::SmallButton(label.c_str())) {
            navigateTo(crumbs[index]);
        }
        ImGui::PopID();
    }
}

void ContentBrowserPanel::reveal(const std::filesystem::path& path) {
    if (m_rootDirectory.empty() || path.empty()) {
        return;
    }

    std::error_code error;
    std::filesystem::path resolved = path.lexically_normal();
    if (resolved.is_relative()) {
        const std::filesystem::path underRoot =
            (m_rootDirectory / resolved).lexically_normal();
        if (std::filesystem::exists(underRoot, error)) {
            resolved = underRoot;
        } else {
            resolved = (m_rootDirectory.parent_path() / path).lexically_normal();
        }
    }
    if (!std::filesystem::exists(resolved, error)) {
        return;
    }

    const std::filesystem::path relative =
        std::filesystem::relative(resolved, m_rootDirectory, error);
    if (error || relative.empty() || relative.generic_string().starts_with("..")) {
        return;
    }

    if (std::filesystem::is_directory(resolved, error)) {
        navigateTo(resolved);
    } else {
        navigateTo(resolved.parent_path());
        m_selectedPath = resolved;
    }
    m_focusRequested = true;
}

void ContentBrowserPanel::refresh() noexcept {
    m_refreshRequested = true;
}

void ContentBrowserPanel::refreshEntries() {
    m_entries.clear();
    m_directoryError.clear();
    std::filesystem::directory_iterator iterator(
        m_currentDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        m_directoryError
    );
    const std::filesystem::directory_iterator end;
    while (!m_directoryError && iterator != end) {
        const std::string filename =
            iterator->path().filename().generic_string();
        if (!filename.empty() && filename.front() != '.') {
            m_entries.push_back(*iterator);
        }
        iterator.increment(m_directoryError);
    }
    std::ranges::sort(
        m_entries,
        [](const auto& left, const auto& right) {
            std::error_code leftError;
            std::error_code rightError;
            const bool leftDirectory = left.is_directory(leftError);
            const bool rightDirectory = right.is_directory(rightError);
            if (leftDirectory != rightDirectory) {
                return leftDirectory;
            }
            return lowercase(left.path().filename().generic_string())
                < lowercase(right.path().filename().generic_string());
        }
    );
    m_directoryWriteTime = std::filesystem::last_write_time(
        m_currentDirectory,
        m_directoryError
    );
    m_nextFilesystemPoll =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    m_refreshRequested = false;
}

std::optional<std::filesystem::path>
ContentBrowserPanel::onImGuiRender() {
    std::optional<std::filesystem::path> sceneToOpen;
    const bool visible = ImGui::Begin("Content Browser");
    if (m_focusRequested) {
        ImGui::SetWindowFocus();
        m_focusRequested = false;
    }
    if (!visible) {
        ImGui::End();
        return sceneToOpen;
    }

    std::error_code error;
    if (m_rootDirectory.empty()) {
        ImGui::TextDisabled("Open or create a project to browse assets.");
        ImGui::End();
        return sceneToOpen;
    }
    if (!std::filesystem::is_directory(m_currentDirectory, error)) {
        ImGui::TextDisabled(
            "Asset directory not found: %s",
            m_rootDirectory.generic_string().c_str()
        );
        ImGui::End();
        return sceneToOpen;
    }

    if (std::chrono::steady_clock::now() >= m_nextFilesystemPoll) {
        std::error_code watchError;
        const auto writeTime = std::filesystem::last_write_time(
            m_currentDirectory,
            watchError
        );
        if (!watchError && writeTime != m_directoryWriteTime) {
            m_textureThumbnails.clear();
            refresh();
        }
        m_nextFilesystemPoll =
            std::chrono::steady_clock::now() + std::chrono::seconds(1);
    }

    ImGui::BeginDisabled(m_backHistory.empty());
    if (ImGui::Button("<")) {
        m_forwardHistory.push_back(m_currentDirectory);
        const auto destination = m_backHistory.back();
        m_backHistory.pop_back();
        navigateTo(destination, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(m_forwardHistory.empty());
    if (ImGui::Button(">")) {
        m_backHistory.push_back(m_currentDirectory);
        const auto destination = m_forwardHistory.back();
        m_forwardHistory.pop_back();
        navigateTo(destination, false);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        refresh();
    }
    ImGui::SameLine();
    if (ImGui::Button("+ Create")) {
        ImGui::OpenPopup("CreateAsset");
    }
    ImGui::SameLine();
    if (ImGui::Button("Favorite")) {
        if (std::ranges::find(m_favorites, m_currentDirectory)
            == m_favorites.end()) {
            m_favorites.push_back(m_currentDirectory);
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ui::scaled(120.0F));
    if (ImGui::BeginCombo("##Favorites", "Favorites")) {
        for (const auto& favorite : m_favorites) {
            const std::string label = currentLocationLabel(
                m_rootDirectory,
                favorite
            );
            if (ImGui::Selectable(label.c_str())) {
                navigateTo(favorite);
            }
        }
        if (m_favorites.empty()) {
            ImGui::TextDisabled("No favorites yet");
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginPopup("CreateAsset")) {
        if (ImGui::MenuItem("Folder")) {
            std::error_code createError;
            std::filesystem::path path = m_currentDirectory / "New Folder";
            for (int suffix = 2; std::filesystem::exists(path, createError); ++suffix) {
                path = m_currentDirectory
                    / ("New Folder " + std::to_string(suffix));
            }
            std::filesystem::create_directory(path, createError);
            if (m_operationHandler) {
                m_operationHandler(
                    createError ? "Folder creation failed" : "Created " + path.filename().generic_string(),
                    !createError
                );
            }
            refresh();
        }
        if (ImGui::MenuItem("Scene")) {
            std::error_code createError;
            std::filesystem::path path = m_currentDirectory / "New Scene.vscene";
            for (int suffix = 2; std::filesystem::exists(path, createError); ++suffix) {
                path = m_currentDirectory
                    / ("New Scene " + std::to_string(suffix) + ".vscene");
            }
            vshade::scene::Scene scene(path.stem().generic_string());
            vshade::scene::SceneSerializer serializer(scene);
            const bool created = serializer.serialize(path);
            if (m_operationHandler) {
                m_operationHandler(
                    created ? "Created " + path.filename().generic_string()
                            : "Scene creation failed",
                    created
                );
            }
            refresh();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ui::scaled(190.0F));
    ImGui::InputTextWithHint(
        "##ContentSearch",
        "Search assets...",
        m_search.data(),
        m_search.size()
    );
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ui::scaled(95.0F));
    ImGui::SliderFloat(
        "##ThumbnailSize",
        &m_thumbnailSize,
        48.0F,
        144.0F,
        "Size %.0f"
    );
    ImGui::SameLine();
    if (ImGui::Button(m_listView ? "Grid" : "List")) {
        m_listView = !m_listView;
    }
    drawBreadcrumbs();
    ImGui::Separator();

    if (m_refreshRequested) {
        refreshEntries();
    }

    const std::string search = lowercase(m_search.data());
    const float thumbnailSize = m_listView ? ui::scaled(28.0F) : m_thumbnailSize;
    const float cardWidth = m_listView
        ? std::max(ImGui::GetContentRegionAvail().x, ui::scaled(240.0F))
        : thumbnailSize + cardPadding * 2.0F;
    const float cardHeight = m_listView
        ? ui::scaled(38.0F)
        : thumbnailSize + cardPadding * 2.0F + labelHeight;
    const float spacing = 10.0F;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {spacing, spacing});
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int columnCount = m_listView ? 1 : std::max(
        1,
        static_cast<int>((availableWidth + spacing) / (cardWidth + spacing))
    );

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    int index = 0;
    for (const auto& entry : m_entries) {
        const std::filesystem::path path = entry.path();
        const std::string filename = path.filename().generic_string();
        if (!search.empty()
            && lowercase(filename).find(search) == std::string::npos) {
            continue;
        }
        std::error_code entryError;
        const bool directory = entry.is_directory(entryError);
        const vshade::asset::AssetType type = directory
            ? vshade::asset::AssetType::Unknown
            : vshade::asset::assetTypeFromExtension(path.extension());
        const bool selected = path == m_selectedPath;

        if (index > 0 && (index % columnCount) != 0) {
            ImGui::SameLine();
        }

        ImGui::PushID(path.generic_string().c_str());
        ImGui::InvisibleButton("##AssetCard", {cardWidth, cardHeight});
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 minimum = ImGui::GetItemRectMin();
        const ImVec2 maximum = ImGui::GetItemRectMax();

        if (ImGui::IsItemClicked()) {
            m_selectedPath = path;
            if (!directory && m_selectionHandler) {
                m_selectionHandler(path);
            }
        }
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (directory) {
                navigateTo(path);
            } else if (type == vshade::asset::AssetType::Scene) {
                sceneToOpen = path;
            }
        }
        if (!directory && ImGui::BeginDragDropSource()) {
            const std::string payloadPath = path.generic_string();
            ImGui::SetDragDropPayload(
                assetDragDropType,
                payloadPath.c_str(),
                payloadPath.size() + 1
            );
            ImGui::TextUnformatted(filename.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginPopupContextItem("AssetActions")) {
            m_selectedPath = path;
            if (ImGui::MenuItem("Rename", "F2")) {
                m_pendingRename = path;
                m_renameBuffer.fill('\0');
                const std::string currentName = filename;
                std::memcpy(
                    m_renameBuffer.data(),
                    currentName.data(),
                    std::min(currentName.size(), m_renameBuffer.size() - 1)
                );
                m_openRename = true;
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                std::error_code copyError;
                const std::string stem = path.stem().generic_string();
                const std::string extension = path.extension().generic_string();
                std::filesystem::path destination = path.parent_path()
                    / (stem + " Copy" + extension);
                for (int suffix = 2;
                     std::filesystem::exists(destination, copyError);
                     ++suffix) {
                    destination = path.parent_path()
                        / (stem + " Copy " + std::to_string(suffix) + extension);
                }
                if (directory) {
                    std::filesystem::copy(
                        path,
                        destination,
                        std::filesystem::copy_options::recursive,
                        copyError
                    );
                } else {
                    std::filesystem::copy_file(path, destination, copyError);
                }
                if (m_operationHandler) {
                    m_operationHandler(
                        copyError ? "Duplicate failed: " + filename
                                  : "Duplicated " + filename,
                        !copyError
                    );
                }
                refresh();
            }
            if (!directory && ImGui::MenuItem("Reimport")) {
                m_textureThumbnails.erase(path.generic_string());
                m_importFailures.erase(path.generic_string());
                try {
                    if (!m_assets) {
                        throw std::runtime_error("No asset manager is available");
                    }
                    const auto reload = [&]<typename Resource>() {
                        const auto reference = m_assets->reference<Resource>(path);
                        if (m_assets->isLoaded(reference.handle())) {
                            m_assets->unload(reference.handle());
                        }
                        static_cast<void>(m_assets->load<Resource>(path));
                    };
                    switch (type) {
                        case vshade::asset::AssetType::Texture:
                            reload.template operator()<vshade::renderer::Texture2D>();
                            break;
                        case vshade::asset::AssetType::Model:
                            reload.template operator()<vshade::renderer::Model>();
                            break;
                        case vshade::asset::AssetType::Audio:
                            reload.template operator()<vshade::audio::AudioClip>();
                            break;
                        case vshade::asset::AssetType::Prefab:
                            reload.template operator()<vshade::scene::Prefab>();
                            break;
                        default:
                            break;
                    }
                    if (m_operationHandler) {
                        m_operationHandler("Reimported " + filename, true);
                    }
                } catch (const std::exception&) {
                    m_importFailures.insert(path.generic_string());
                    if (m_operationHandler) {
                        m_operationHandler("Reimport failed: " + filename, false);
                    }
                }
                refresh();
            }
            if (ImGui::MenuItem("Copy Project-Relative Path")) {
                std::error_code relativeError;
                const std::string relative = std::filesystem::relative(
                    path,
                    m_rootDirectory.parent_path(),
                    relativeError
                ).generic_string();
                ImGui::SetClipboardText(
                    relativeError ? path.generic_string().c_str() : relative.c_str()
                );
            }
#if defined(_WIN32)
            if (ImGui::MenuItem("Reveal in Explorer")) {
                const std::wstring parameters = L"/select,\""
                    + path.wstring() + L"\"";
                ShellExecuteW(
                    nullptr,
                    L"open",
                    L"explorer.exe",
                    parameters.c_str(),
                    nullptr,
                    SW_SHOWNORMAL
                );
            }
#endif
            ImGui::Separator();
            ui::pushDestructiveTextStyle();
            if (ImGui::MenuItem("Delete")) {
                m_pendingDelete = path;
                m_openDelete = true;
            }
            ui::popDestructiveTextStyle();
            ImGui::EndPopup();
        }
        if (hovered) {
            ImGui::SetTooltip("%s\n%s", filename.c_str(), typeName(directory, type));
        }

        const ImU32 cardColor = selected
            ? IM_COL32(48, 54, 66, 255)
            : hovered
                ? IM_COL32(42, 46, 54, 255)
                : IM_COL32(34, 36, 42, 255);
        drawList->AddRectFilled(minimum, maximum, cardColor, cardRounding);
        if (selected) {
            drawList->AddRect(
                minimum,
                maximum,
                IM_COL32(70, 150, 255, 255),
                cardRounding,
                0,
                1.5F
            );
        }

        const ImVec2 previewMinimum{
            minimum.x + cardPadding,
            minimum.y + cardPadding
        };
        const ImVec2 previewMaximum{
            m_listView
                ? previewMinimum.x + thumbnailSize
                : maximum.x - cardPadding,
            minimum.y + cardPadding + thumbnailSize
        };
        std::shared_ptr<vshade::renderer::Texture2D> previewTexture;
        if (!directory && type == vshade::asset::AssetType::Texture && m_assets) {
            const std::string key = path.generic_string();
            if (const auto found = m_textureThumbnails.find(key);
                found != m_textureThumbnails.end()) {
                previewTexture = found->second;
            } else if (!m_importFailures.contains(key)) {
                try {
                    previewTexture = m_assets->loadResource<
                        vshade::renderer::Texture2D>(path).shared();
                    m_textureThumbnails.emplace(key, previewTexture);
                } catch (const std::exception&) {
                    m_importFailures.insert(key);
                }
            }
        }
        const auto icon = previewTexture
            ? previewTexture
            : iconForEntry(directory, type);
        if (icon) {
            const ImTextureID textureId =
                static_cast<ImTextureID>(icon->rendererId());
            const float iconPadding = previewTexture ? 0.0F : ui::scaled(8.0F);
            drawList->AddImage(
                ImTextureRef{textureId},
                {
                    previewMinimum.x + iconPadding,
                    previewMinimum.y + iconPadding,
                },
                {
                    previewMaximum.x - iconPadding,
                    previewMaximum.y - iconPadding,
                }
            );
        }

        const float labelLeft = m_listView
            ? previewMaximum.x + ui::scaled(8.0F)
            : minimum.x + cardPadding;
        const float labelTop = m_listView
            ? minimum.y + (cardHeight - ImGui::GetTextLineHeight()) * 0.5F
            : previewMaximum.y + 4.0F;
        drawList->AddCircleFilled(
            {labelLeft + 4.0F, labelTop + 8.0F},
            3.5F,
            typeColor(directory, type)
        );
        const std::string label = ellipsize(filename, cardWidth - cardPadding * 2.0F - 14.0F);
        drawList->AddText(
            {labelLeft + 12.0F, labelTop},
            ImGui::GetColorU32(ImGuiCol_Text),
            label.c_str()
        );
        if (m_importFailures.contains(path.generic_string())) {
            const ImVec2 badgeCenter{
                maximum.x - ui::scaled(10.0F),
                minimum.y + ui::scaled(10.0F),
            };
            drawList->AddCircleFilled(
                badgeCenter,
                ui::scaled(7.0F),
                ImGui::GetColorU32(ui::color(ui::ColorRole::Error)),
                16
            );
            drawList->AddText(
                {badgeCenter.x - ui::scaled(2.0F), badgeCenter.y - ui::scaled(7.0F)},
                IM_COL32_WHITE,
                "!"
            );
        }

        ImGui::PopID();
        ++index;
    }
    ImGui::PopStyleVar();

    if (m_openRename) {
        ImGui::OpenPopup("Rename Asset");
        m_openRename = false;
    }
    if (ImGui::BeginPopupModal(
            "Rename Asset",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
        ImGui::InputText(
            "Name",
            m_renameBuffer.data(),
            m_renameBuffer.size()
        );
        const std::filesystem::path destination =
            m_pendingRename.parent_path() / m_renameBuffer.data();
        std::error_code destinationError;
        const bool destinationExists =
            std::filesystem::exists(destination, destinationError);
        const bool validName = m_renameBuffer[0] != '\0'
            && destination != m_pendingRename
            && !destinationExists
            && !destinationError;
        if (destinationExists) {
            ImGui::TextColored(
                ui::color(ui::ColorRole::Warning),
                "An item with that name already exists."
            );
        }
        ImGui::BeginDisabled(!validName);
        if (ImGui::Button("Rename")) {
            std::error_code renameError;
            if (!std::filesystem::exists(destination, renameError)) {
                std::filesystem::rename(
                    m_pendingRename,
                    destination,
                    renameError
                );
            }
            if (m_operationHandler) {
                m_operationHandler(
                    renameError ? "Rename failed"
                                : "Renamed to " + destination.filename().generic_string(),
                    !renameError
                );
            }
            m_pendingRename.clear();
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_pendingRename.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_openDelete) {
        ImGui::OpenPopup("Delete Asset?");
        m_openDelete = false;
    }
    if (ImGui::BeginPopupModal(
            "Delete Asset?",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize
        )) {
        ImGui::TextWrapped(
            "Permanently delete '%s'?",
            m_pendingDelete.filename().generic_string().c_str()
        );
        ImGui::TextColored(
            ui::color(ui::ColorRole::Warning),
            "References to this asset may become missing."
        );
        ui::pushDestructiveButtonStyle();
        if (ImGui::Button("Delete")) {
            std::error_code deleteError;
            std::filesystem::remove_all(m_pendingDelete, deleteError);
            if (m_operationHandler) {
                m_operationHandler(
                    deleteError ? "Asset deletion failed"
                                : "Deleted asset",
                    !deleteError
                );
            }
            m_textureThumbnails.erase(m_pendingDelete.generic_string());
            m_pendingDelete.clear();
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ui::popDestructiveButtonStyle();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_pendingDelete.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_entries.empty() && !m_directoryError) {
        ImGui::TextDisabled("This folder is empty.");
    }
    if (m_directoryError) {
        ImGui::TextDisabled("Unable to read this directory.");
    }

    ImGui::End();
    return sceneToOpen;
}

} // namespace editor
