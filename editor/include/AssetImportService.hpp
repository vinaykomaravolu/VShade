#pragma once

#include <filesystem>
#include <string>

namespace vshade::project {
class Project;
}

namespace editor {

enum class AssetCollisionPolicy {
    Cancel,
    Replace,
    KeepBoth,
};

struct AssetImportResult {
    std::filesystem::path destination;
    std::string error;
    bool imported = false;
    bool cancelled = false;

    [[nodiscard]] explicit operator bool() const noexcept {
        return imported;
    }
};

/** Copies an external asset into the appropriate project-owned directory. */
class AssetImportService final {
public:
    AssetImportService() = delete;

    [[nodiscard]] static AssetImportResult import(
        const vshade::project::Project& project,
        const std::filesystem::path& source,
        AssetCollisionPolicy collisionPolicy
    );
};

} // namespace editor
