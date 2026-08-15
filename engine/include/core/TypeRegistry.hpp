#pragma once

#include "scene/SceneComponentRegistry.hpp"
#include "script/NativeScriptRegistry.hpp"

#include <concepts>
#include <string>
#include <utility>

namespace vshade::core {

/** @brief One startup surface for persistent game component and script types. */
class TypeRegistry final {
public:
    explicit TypeRegistry(script::NativeScriptRegistry& scripts) noexcept
        : m_scripts(&scripts) {}

    template<typename Component>
        requires std::copy_constructible<Component>
    TypeRegistry& component(std::string stableName) {
        scene::SceneComponentRegistry::registerComponent<Component>(
            std::move(stableName)
        );
        return *this;
    }

    template<typename Script>
        requires std::derived_from<Script, ::vshade::script::NativeScript> &&
                 std::default_initializable<Script>
    TypeRegistry& script(std::string stableName) {
        m_scripts->registerType<Script>(std::move(stableName));
        return *this;
    }

private:
    script::NativeScriptRegistry* m_scripts = nullptr;
};

} // namespace vshade::core
