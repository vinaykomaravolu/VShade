#pragma once

#include "script/ScriptTypes.hpp"

#include <string>
#include <vector>

namespace vshade::scene {

/**
 * @brief Serializable reference to one script attached to an entity.
 *
 * Native C++ bindings use the stable name registered with
 * NativeScriptRegistry. Other backends may interpret the same name as an
 * assembly type, module, or source asset when those backends are introduced.
 */
struct ScriptBinding {
    script::ScriptBackend backend = script::ScriptBackend::NativeCpp;
    std::string typeName;
    bool enabled = true;
};

/** @brief Ordered collection of scripts attached to one entity. */
struct ScriptComponent {
    std::vector<ScriptBinding> scripts;
};

} // namespace vshade::scene
