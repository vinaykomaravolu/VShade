#pragma once

#include <memory>

namespace vshade::scene {

class Scene;

/** @brief Immutable reusable scene template loaded as an asset. */
class Prefab final {
public:
    explicit Prefab(std::shared_ptr<const Scene> scene);
    [[nodiscard]] const Scene& scene() const noexcept;

private:
    std::shared_ptr<const Scene> m_scene;
};

} // namespace vshade::scene
