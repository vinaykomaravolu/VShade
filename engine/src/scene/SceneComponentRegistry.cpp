#include "scene/SceneComponentRegistry.hpp"

#include <algorithm>
#include <stdexcept>

namespace vshade::scene {

std::vector<SceneComponentRegistry::Handler>& SceneComponentRegistry::mutableHandlers() {
    static std::vector<SceneComponentRegistry::Handler> handlers;
    return handlers;
}

void SceneComponentRegistry::add(Handler handler) {
    if (handler.name.empty()) {
        throw std::invalid_argument("A serialized component name cannot be empty");
    }
    if (handler.name == "UUID" || handler.name == "Tag" ||
        handler.name == "Transform" || handler.name == "SpriteRenderer") {
        throw std::invalid_argument("A custom component cannot use a built-in component name");
    }

    auto& existing = mutableHandlers();
    if (std::ranges::any_of(existing, [&handler](const Handler& candidate) {
            return candidate.type == handler.type || candidate.name == handler.name;
        })) {
        throw std::invalid_argument("Component type or serialized name is already registered");
    }
    existing.push_back(std::move(handler));
}

const std::vector<SceneComponentRegistry::Handler>& SceneComponentRegistry::handlers() {
    return mutableHandlers();
}

} // namespace vshade::scene
