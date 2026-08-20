#include "AssetImportService.hpp"

#include <asset/Asset.hpp>
#include <project/Project.hpp>

#include <system_error>

namespace editor {
namespace {

[[nodiscard]] std::filesystem::path destinationDirectory(
    const vshade::project::Project& project,
    const vshade::asset::AssetType type
) {
    switch (type) {
        case vshade::asset::AssetType::Model:
            return project.assetDirectory() / "models";
        case vshade::asset::AssetType::Texture:
            return project.assetDirectory() / "textures";
        case vshade::asset::AssetType::Audio:
            return project.assetDirectory() / "audio";
        case vshade::asset::AssetType::Prefab:
            return project.prefabDirectory();
        default:
            return {};
    }
}

[[nodiscard]] std::filesystem::path keepBothPath(
    const std::filesystem::path& initial
) {
    for (std::size_t suffix = 1; suffix < 10000; ++suffix) {
        const std::filesystem::path candidate = initial.parent_path()
            / (initial.stem().string() + " (" + std::to_string(suffix) + ")"
               + initial.extension().string());
        std::error_code error;
        if (!std::filesystem::exists(candidate, error) && !error) {
            return candidate;
        }
    }
    return {};
}

} // namespace

AssetImportResult AssetImportService::import(
    const vshade::project::Project& project,
    const std::filesystem::path& source,
    const AssetCollisionPolicy collisionPolicy
) {
    AssetImportResult result;
    std::error_code error;
    if (!std::filesystem::is_regular_file(source, error) || error) {
        result.error = "The selected asset is not a readable file";
        return result;
    }

    const auto type = vshade::asset::assetTypeFromExtension(source.extension());
    const std::filesystem::path directory = destinationDirectory(project, type);
    if (directory.empty()) {
        result.error = "The selected asset type is not supported";
        return result;
    }

    std::filesystem::create_directories(directory, error);
    if (error) {
        result.error = error.message();
        return result;
    }

    std::filesystem::path destination = directory / source.filename();
    if (std::filesystem::exists(destination, error)) {
        if (error) {
            result.error = error.message();
            return result;
        }
        error.clear();
        if (std::filesystem::equivalent(source, destination, error) && !error) {
            result.destination = destination;
            result.imported = true;
            return result;
        }
        if (collisionPolicy == AssetCollisionPolicy::Cancel) {
            result.cancelled = true;
            return result;
        }
        if (collisionPolicy == AssetCollisionPolicy::KeepBoth) {
            destination = keepBothPath(destination);
            if (destination.empty()) {
                result.error = "Unable to choose an unused destination filename";
                return result;
            }
        }
    }

    const auto options = collisionPolicy == AssetCollisionPolicy::Replace
        ? std::filesystem::copy_options::overwrite_existing
        : std::filesystem::copy_options::none;
    if (!std::filesystem::copy_file(source, destination, options, error)) {
        result.error = error ? error.message() : "The asset could not be copied";
        return result;
    }

    result.destination = std::move(destination);
    result.imported = true;
    return result;
}

} // namespace editor
