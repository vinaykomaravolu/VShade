#pragma once

#include "asset/AssetHandle.hpp"

#include <filesystem>
#include <utility>

namespace vshade::asset {

/** @brief Serializable typed asset identity with a resolvable source location. */
template<typename Resource>
class AssetReference final {
public:
    AssetReference() = default;

    AssetReference(
        const AssetHandle<Resource> handle,
        std::filesystem::path sourcePath
    ) : m_handle(handle), m_sourcePath(std::move(sourcePath)) {}

    /** @brief Returns the stable typed identifier. */
    [[nodiscard]] AssetHandle<Resource> handle() const noexcept {
        return m_handle;
    }

    /** @brief Returns the normalized project-relative source location. */
    [[nodiscard]] const std::filesystem::path& sourcePath() const noexcept {
        return m_sourcePath;
    }

    /** @brief Reports whether both an identifier and source location are present. */
    [[nodiscard]] bool valid() const noexcept {
        return m_handle.valid() && !m_sourcePath.empty();
    }

    explicit operator bool() const noexcept { return valid(); }
    bool operator==(const AssetReference&) const = default;

private:
    AssetHandle<Resource> m_handle;
    std::filesystem::path m_sourcePath;
};

} // namespace vshade::asset
