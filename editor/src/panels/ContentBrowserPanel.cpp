#include "panels/ContentBrowserPanel.hpp"
#include "EditorIcons.hpp"
#include "widgets/AssetSelector.hpp"

#include <asset/Asset.hpp>
#include <renderer/Texture.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <imgui.h>

namespace editor {
namespace {

constexpr float thumbnailSize = 88.0F;
constexpr float cardPadding = 8.0F;
constexpr float labelHeight = 22.0F;
constexpr float cardRounding = 8.0F;
constexpr float cardWidth = thumbnailSize + cardPadding * 2.0F;
constexpr float cardHeight = thumbnailSize + cardPadding * 2.0F + labelHeight;

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
    refresh();
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
        m_currentDirectory = resolved;
        m_selectedPath.clear();
    } else {
        m_currentDirectory = resolved.parent_path();
        m_selectedPath = resolved;
    }
    m_focusRequested = true;
    refresh();
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

    if (m_currentDirectory != m_rootDirectory) {
        if (ImGui::Button("<-")) {
            const std::filesystem::path parent =
                m_currentDirectory.parent_path();
            m_currentDirectory = parent.empty()
                ? m_rootDirectory
                : parent;
            m_selectedPath.clear();
            refresh();
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Refresh")) {
        refresh();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(
        currentLocationLabel(m_rootDirectory, m_currentDirectory).c_str()
    );
    ImGui::Separator();

    if (m_refreshRequested) {
        refreshEntries();
    }

    const float spacing = 10.0F;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {spacing, spacing});
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const int columnCount = std::max(
        1,
        static_cast<int>((availableWidth + spacing) / (cardWidth + spacing))
    );

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    int index = 0;
    for (const auto& entry : m_entries) {
        const std::filesystem::path path = entry.path();
        const std::string filename = path.filename().generic_string();
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
        }
        if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (directory) {
                m_currentDirectory = path.lexically_normal();
                m_selectedPath.clear();
                refresh();
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
            maximum.x - cardPadding,
            minimum.y + cardPadding + thumbnailSize
        };
        if (const auto icon = iconForEntry(directory, type)) {
            const ImTextureID textureId =
                static_cast<ImTextureID>(icon->rendererId());
            constexpr float iconPadding = 8.0F;
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

        const float labelLeft = minimum.x + cardPadding;
        const float labelTop = previewMaximum.y + 4.0F;
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

        ImGui::PopID();
        ++index;
    }
    ImGui::PopStyleVar();

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
