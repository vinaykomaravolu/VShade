#include "widgets/AssetSelector.hpp"

#include <asset/Asset.hpp>
#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <core/Log.hpp>
#include <renderer/Model.hpp>
#include <renderer/Texture.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <exception>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>

#include <ImGuiFileDialog.h>
#include <imgui.h>

namespace editor {
namespace {

std::filesystem::path g_searchDirectory;
std::function<void()> g_catalogChanged;

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

void discoverAssetsInDirectory(
    vshade::asset::AssetManager& assets,
    const std::filesystem::path& directory
) {
    std::error_code error;
    if (directory.empty()
        || !std::filesystem::is_directory(directory, error)) {
        return;
    }

    std::filesystem::recursive_directory_iterator iterator(
        directory,
        std::filesystem::directory_options::skip_permission_denied,
        error
    );
    const std::filesystem::recursive_directory_iterator end;
    while (!error && iterator != end) {
        std::error_code entryError;
        const std::filesystem::path path = iterator->path();
        const bool regularFile = iterator->is_regular_file(entryError);
        iterator.increment(error);
        if (!regularFile) {
            continue;
        }

        try {
            switch (vshade::asset::assetTypeFromExtension(path.extension())) {
                case vshade::asset::AssetType::Model:
                    static_cast<void>(
                        assets.reference<vshade::renderer::Model>(path)
                    );
                    break;
                case vshade::asset::AssetType::Texture:
                    static_cast<void>(
                        assets.reference<vshade::renderer::Texture2D>(path)
                    );
                    break;
                case vshade::asset::AssetType::Audio:
                    static_cast<void>(
                        assets.reference<vshade::audio::AudioClip>(path)
                    );
                    break;
                default:
                    break;
            }
        } catch (const std::exception& discoverError) {
            ENGINE_WARN(
                "Skipped asset '{}': {}",
                path.generic_string(),
                discoverError.what()
            );
        }
    }

    if (g_catalogChanged) {
        g_catalogChanged();
    }
}

[[nodiscard]] bool extensionMatches(
    const std::filesystem::path& path,
    const std::initializer_list<std::string_view> extensions
) {
    const std::string extension = lowercase(path.extension().generic_string());
    for (const std::string_view candidate : extensions) {
        if (extension == candidate) {
            return true;
        }
    }
    return false;
}

template<typename Resource>
bool assignDroppedAsset(
    vshade::asset::AssetReference<Resource>& reference,
    vshade::asset::AssetManager& assets,
    const std::initializer_list<std::string_view> extensions
) {
    if (!ImGui::BeginDragDropTarget()) {
        return false;
    }

    bool changed = false;
    if (const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(assetDragDropType)) {
        const std::filesystem::path path{
            static_cast<const char*>(payload->Data)
        };
        if (!extensionMatches(path, extensions)) {
            ENGINE_WARN(
                "Asset '{}' cannot be assigned to this field",
                path.generic_string()
            );
        } else {
            try {
                reference = assets.reference<Resource>(path);
                changed = true;
            } catch (const std::exception& error) {
                ENGINE_ERROR(
                    "Failed to assign asset '{}': {}",
                    path.generic_string(),
                    error.what()
                );
            }
        }
    }
    ImGui::EndDragDropTarget();
    return changed;
}

template<typename Resource>
bool drawTypedAssetSelector(
    const char* label,
    const char* typeId,
    const char* dialogTitle,
    const char* filters,
    const std::initializer_list<std::string_view> extensions,
    vshade::asset::AssetReference<Resource>& reference,
    vshade::asset::AssetManager& assets
) {
    const std::string stateKey = std::string(typeId) + ":" + label;
    const std::string dialogKey = "ImportAsset:" + stateKey;
    static std::unordered_map<std::string, std::array<char, 128>> searches;
    auto& search = searches[stateKey];

    ImGui::PushID(stateKey.c_str());
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    const std::string currentName = reference
        ? reference.sourcePath().filename().generic_string()
        : "None";
    const std::string fieldLabel = currentName + "  \xE2\x96\xBC";
    if (ImGui::Button(fieldLabel.c_str())) {
        discoverAssetsInDirectory(assets, g_searchDirectory);
        ImGui::OpenPopup("AssetSelectorPopup");
    }
    ImGui::EndGroup();

    bool changed = assignDroppedAsset(reference, assets, extensions);
    bool importRequested = false;
    if (ImGui::BeginPopup("AssetSelectorPopup")) {
        ImGui::SetNextItemWidth(280.0F);
        ImGui::InputTextWithHint(
            "##Search",
            "Search assets...",
            search.data(),
            search.size()
        );
        ImGui::Separator();

        if (ImGui::Selectable("None", !reference)) {
            reference = {};
            changed = true;
            ImGui::CloseCurrentPopup();
        }

        const std::string query = lowercase(search.data());
        const auto knownAssets = assets.knownAssets<Resource>();
        std::size_t listed = 0;
        for (const auto& asset : knownAssets) {
            const std::string name =
                asset.sourcePath.filename().generic_string();
            if (!query.empty()
                && lowercase(name).find(query) == std::string::npos) {
                continue;
            }

            ImGui::PushID(asset.sourcePath.generic_string().c_str());
            const bool selected =
                reference && reference.handle().id() == asset.id;
            if (ImGui::Selectable(name.c_str(), selected)) {
                reference = assets.reference<Resource>(asset.sourcePath);
                changed = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
            ++listed;
        }
        if (knownAssets.empty()) {
            ImGui::TextDisabled("No assets of this type in the project.");
        } else if (listed == 0) {
            ImGui::TextDisabled("No matching assets.");
        }

        ImGui::Separator();
        if (ImGui::Button("Import New...")) {
            importRequested = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (importRequested) {
        IGFD::FileDialogConfig config;
        config.path = g_searchDirectory.empty()
            ? "."
            : g_searchDirectory.generic_string();
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::Instance()->OpenDialog(
            dialogKey,
            dialogTitle,
            filters,
            config
        );
    }

    if (ImGuiFileDialog::Instance()->Display(dialogKey)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string selectedPath =
                ImGuiFileDialog::Instance()->GetFilePathName();
            try {
                static_cast<void>(assets.load<Resource>(selectedPath));
                reference = assets.reference<Resource>(selectedPath);
                changed = true;
            } catch (const std::exception& error) {
                ENGINE_ERROR("Failed to import asset '{}': {}", selectedPath, error.what());
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    ImGui::PopID();
    return changed;
}

} // namespace

void AssetSelector::setSearchDirectory(std::filesystem::path directory) {
    g_searchDirectory = std::move(directory).lexically_normal();
}

void AssetSelector::setCatalogChangedCallback(std::function<void()> callback) {
    g_catalogChanged = std::move(callback);
}

void AssetSelector::discover(vshade::asset::AssetManager& assets) {
    discoverAssetsInDirectory(assets, g_searchDirectory);
}

bool AssetSelector::acceptDroppedAsset(
    vshade::asset::AssetReference<vshade::renderer::Texture2D>& reference,
    vshade::asset::AssetManager& assets
) {
    return assignDroppedAsset(
        reference,
        assets,
        {".png", ".jpg", ".jpeg", ".bmp", ".tga"}
    );
}

bool AssetSelector::acceptDroppedAsset(
    vshade::asset::AssetReference<vshade::renderer::Model>& reference,
    vshade::asset::AssetManager& assets
) {
    return assignDroppedAsset(reference, assets, {".glb", ".gltf"});
}

bool AssetSelector::acceptDroppedAsset(
    vshade::asset::AssetReference<vshade::audio::AudioClip>& reference,
    vshade::asset::AssetManager& assets
) {
    return assignDroppedAsset(
        reference,
        assets,
        {".wav", ".mp3", ".flac", ".ogg"}
    );
}

bool AssetSelector::draw(
    const char* label,
    vshade::asset::AssetReference<vshade::renderer::Texture2D>& reference,
    vshade::asset::AssetManager& assets
) {
    return drawTypedAssetSelector(
        label,
        "Texture",
        "Import Texture",
        ".png,.jpg,.jpeg,.bmp,.tga",
        {".png", ".jpg", ".jpeg", ".bmp", ".tga"},
        reference,
        assets
    );
}

bool AssetSelector::draw(
    const char* label,
    vshade::asset::AssetReference<vshade::renderer::Model>& reference,
    vshade::asset::AssetManager& assets
) {
    return drawTypedAssetSelector(
        label,
        "Model",
        "Import Model",
        ".glb,.gltf",
        {".glb", ".gltf"},
        reference,
        assets
    );
}

bool AssetSelector::draw(
    const char* label,
    vshade::asset::AssetReference<vshade::audio::AudioClip>& reference,
    vshade::asset::AssetManager& assets
) {
    return drawTypedAssetSelector(
        label,
        "AudioClip",
        "Import Audio Clip",
        ".wav,.mp3,.flac,.ogg",
        {".wav", ".mp3", ".flac", ".ogg"},
        reference,
        assets
    );
}

} // namespace editor
