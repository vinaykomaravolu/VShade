#pragma once

#include "asset/Asset.hpp"

namespace vshade::asset {

class AssetManager;

/** @brief Lightweight, serializable identifier for an asset of type @p Resource. */
template<typename Resource>
class AssetHandle final {
public:
    /** @brief Creates an invalid handle. */
    constexpr AssetHandle() noexcept = default;

    /** @brief Returns the stable asset identifier. */
    [[nodiscard]] constexpr AssetId id() const noexcept {
        return m_id;
    }

    /** @brief Reports whether this handle contains a nonzero identifier. */
    [[nodiscard]] constexpr bool valid() const noexcept {
        return m_id != invalidAssetId;
    }

    constexpr explicit operator bool() const noexcept {
        return valid();
    }

    bool operator==(const AssetHandle&) const = default;

private:
    friend class AssetManager;

    explicit constexpr AssetHandle(const AssetId id) noexcept
        : m_id(id) {}

    AssetId m_id = invalidAssetId;
};

} // namespace vshade::asset
