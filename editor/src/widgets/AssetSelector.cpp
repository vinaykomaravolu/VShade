#include "widgets/AssetSelector.hpp"

#include <asset/AssetManager.hpp>
#include <audio/AudioClip.hpp>
#include <core/Log.hpp>
#include <renderer/Model.hpp>
#include <renderer/Texture.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <string>
#include <unordered_map>

#include <ImGuiFileDialog.h>
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

template<typename Resource>
bool drawTypedAssetSelector(
    const char* label,
    const char* typeId,
    const char* dialogTitle,
    const char* filters,
    vshade::asset::AssetReference<Resource>& reference,
    vshade::asset::AssetManager& assets
) {
    const std::string stateKey = std::string(typeId) + ":" + label;
    const std::string dialogKey = "ImportAsset:" + stateKey;
    static std::unordered_map<std::string, std::array<char, 128>> searches;
    auto& search = searches[stateKey];

    ImGui::PushID(stateKey.c_str());
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    const std::string currentName = reference
        ? reference.sourcePath().filename().generic_string()
        : "None";
    const std::string fieldLabel = currentName + "  \xE2\x96\xBC";
    if (ImGui::Button(fieldLabel.c_str())) {
        ImGui::OpenPopup("AssetSelectorPopup");
    }

    bool changed = false;
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
        config.path = ".";
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
        reference,
        assets
    );
}

} // namespace editor
