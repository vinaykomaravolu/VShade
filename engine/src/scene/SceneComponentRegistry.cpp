#include "scene/SceneComponentRegistry.hpp"

#include "scene/Components.hpp"

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
        handler.name == "Transform" || handler.name == "SpriteRenderer" ||
        handler.name == "AudioSource" || handler.name == "AudioListener" ||
        handler.name == "Light" || handler.name == "Scripts" ||
        handler.type == entt::type_hash<ScriptComponent>::value()) {
        throw std::invalid_argument(
            "A custom component cannot reuse a built-in component type or name"
        );
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
