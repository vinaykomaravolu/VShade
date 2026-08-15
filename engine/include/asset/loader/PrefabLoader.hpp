#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::scene { class Prefab; }

namespace vshade::asset {

/** @brief Loads a reusable prefab from the same stable JSON format as scenes. */
class PrefabLoader final : public AssetLoader<scene::Prefab> {
public:
    [[nodiscard]] std::shared_ptr<scene::Prefab> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
