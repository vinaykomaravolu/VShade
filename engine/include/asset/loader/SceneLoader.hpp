#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::scene {
class Scene;
}

namespace vshade::asset {

/** @brief Loads serialized scene files into runtime scene resources. */
class SceneLoader final : public AssetLoader<scene::Scene> {
public:
    SceneLoader() = default;
    ~SceneLoader() override = default;

    /** @brief Deserializes a scene file into a newly owned scene resource. */
    [[nodiscard]] std::shared_ptr<scene::Scene> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
