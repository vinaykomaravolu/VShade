#include "panels/ContentBrowserPanel.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <imgui.h>

namespace editor {
namespace {

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

} // namespace

ContentBrowserPanel::ContentBrowserPanel(
    std::filesystem::path assetDirectory
) {
    setAssetDirectory(std::move(assetDirectory));
}

void ContentBrowserPanel::setAssetDirectory(
    std::filesystem::path assetDirectory
) {
    m_assetDirectory = std::move(assetDirectory).lexically_normal();
    m_currentDirectory = m_assetDirectory;
}

std::optional<std::filesystem::path>
ContentBrowserPanel::onImGuiRender() {
    std::optional<std::filesystem::path> sceneToOpen;
    const bool visible = ImGui::Begin("Content Browser");
    if (!visible) {
        ImGui::End();
        return sceneToOpen;
    }

    std::error_code error;
    if (m_assetDirectory.empty()) {
        ImGui::TextDisabled("Open or create a project to browse assets.");
        ImGui::End();
        return sceneToOpen;
    }
    if (!std::filesystem::is_directory(m_currentDirectory, error)) {
        ImGui::TextDisabled(
            "Asset directory not found: %s",
            m_assetDirectory.generic_string().c_str()
        );
        ImGui::End();
        return sceneToOpen;
    }

    if (m_currentDirectory != m_assetDirectory) {
        if (ImGui::Button("<-")) {
            const std::filesystem::path parent =
                m_currentDirectory.parent_path();
            m_currentDirectory = parent.empty()
                ? m_assetDirectory
                : parent;
        }
        ImGui::SameLine();
    }
    ImGui::TextUnformatted(m_currentDirectory.generic_string().c_str());
    ImGui::Separator();

    std::vector<std::filesystem::directory_entry> entries;
    std::filesystem::directory_iterator iterator(
        m_currentDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        error
    );
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end) {
        entries.push_back(*iterator);
        iterator.increment(error);
    }
    std::ranges::sort(
        entries,
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

    for (const auto& entry : entries) {
        const std::filesystem::path path = entry.path();
        const std::string filename = path.filename().generic_string();
        std::error_code entryError;
        const bool directory = entry.is_directory(entryError);
        const std::string label = directory
            ? "[Folder] " + filename
            : filename;

        ImGui::PushID(path.generic_string().c_str());
        ImGui::Selectable(label.c_str());

        if (ImGui::IsItemHovered()
            && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (directory) {
                m_currentDirectory = path.lexically_normal();
            } else if (lowercase(path.extension().generic_string())
                == ".vscene") {
                sceneToOpen = path;
            }
        }

        if (!directory && ImGui::BeginDragDropSource()) {
            const std::string payloadPath = path.generic_string();
            ImGui::SetDragDropPayload(
                "VShadeAsset",
                payloadPath.c_str(),
                payloadPath.size() + 1
            );
            ImGui::TextUnformatted(filename.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }

    if (error) {
        ImGui::TextDisabled("Unable to read this directory.");
    }

    ImGui::End();
    return sceneToOpen;
}

} // namespace editor
