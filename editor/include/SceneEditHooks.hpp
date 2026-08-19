#pragma once

#include <functional>

namespace editor {

/** @brief Begin/commit/revert callbacks for editor scene mutations. */
struct SceneEditHooks {
    std::function<void()> begin;
    std::function<void()> commit;
    std::function<void()> revert;
    std::function<void()> cancel;
};

} // namespace editor
