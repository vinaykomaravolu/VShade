#include "asset/loader/ShaderLoader.hpp"

#include "renderer/shader.hpp"

#include <memory>
#include <stdexcept>

namespace vshade::asset {

std::shared_ptr<renderer::Shader> ShaderLoader::load(
    const std::filesystem::path& path
) const {
    if (path.extension() != ".vs") {
        throw std::invalid_argument("A shader asset path must identify a .vs file");
    }

    std::filesystem::path fragmentPath = path;
    fragmentPath.replace_extension(".fs");
    return std::make_shared<renderer::Shader>(renderer::Shader::fromFiles(
        path.stem().string(),
        path,
        fragmentPath
    ));
}

} // namespace vshade::asset
