#include "asset/loader/TextureLoader.hpp"

#include "renderer/texture.hpp"

#include <memory>

namespace vshade::asset {

std::shared_ptr<renderer::Texture2D> TextureLoader::load(
    const std::filesystem::path& path
) const {
    return std::make_shared<renderer::Texture2D>(
        renderer::Texture2D::fromFile(path)
    );
}

} // namespace vshade::asset
