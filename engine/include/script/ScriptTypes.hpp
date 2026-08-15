#pragma once

#include <cstdint>

namespace vshade::script {

/** @brief Runtime responsible for executing a script binding. */
enum class ScriptBackend : std::uint8_t {
    NativeCpp,
    CSharp,
    Lua,
    Custom,
};

} // namespace vshade::script
