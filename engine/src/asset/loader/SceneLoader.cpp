#include "asset/loader/SceneLoader.hpp"

#include "scene/Scene.hpp"
#include "scene/SceneSerializer.hpp"

#include <memory>
#include <stdexcept>

namespace vshade::asset {

std::shared_ptr<scene::Scene> SceneLoader::load(
    const std::filesystem::path& path
) const {
    auto loaded = std::make_shared<scene::Scene>();
    scene::SceneSerializer serializer(*loaded);
    if (!serializer.deserialize(path)) {
        throw std::runtime_error(
            "Failed to load scene asset '" + path.string() + "': " +
            serializer.lastError()
        );
    }
    return loaded;
}

} // namespace vshade::asset
