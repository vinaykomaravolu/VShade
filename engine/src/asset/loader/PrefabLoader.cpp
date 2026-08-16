#include "asset/loader/PrefabLoader.hpp"

#include "asset/loader/SceneLoader.hpp"
#include "scene/Prefab.hpp"

namespace vshade::asset {

std::shared_ptr<scene::Prefab> PrefabLoader::load(
    const std::filesystem::path& path
) const {
    return std::make_shared<scene::Prefab>(SceneLoader{}.load(path));
}

} // namespace vshade::asset
