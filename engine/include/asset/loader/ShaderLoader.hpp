#pragma once

#include "asset/AssetLoader.hpp"

namespace vshade::renderer {
class Shader;
}

namespace vshade::asset {

/** @brief Loads paired GLSL vertex and fragment files into shader programs. */
class ShaderLoader final : public AssetLoader<renderer::Shader> {
public:
    ShaderLoader() = default;
    ~ShaderLoader() override = default;

    /**
     * @brief Loads a .vs vertex file and its same-stem .fs fragment file.
     * @throws std::invalid_argument If @p path does not have the .vs extension.
     */
    [[nodiscard]] std::shared_ptr<renderer::Shader> load(
        const std::filesystem::path& path
    ) const override;
};

} // namespace vshade::asset
