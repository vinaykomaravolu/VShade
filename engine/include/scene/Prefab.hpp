#pragma once

#include "core/Result.hpp"
#include "scene/Entity.hpp"

#include <filesystem>
#include <memory>

namespace vshade::scene {

class Scene;

/** @brief Immutable reusable entity template stored as scene JSON. */
class Prefab final {
public:
    explicit Prefab(std::shared_ptr<const Scene> scene);

    /**
     * @brief Captures an entity and its descendants as a prefab.
     * @throws std::invalid_argument If @p root is invalid or belongs to another scene.
     */
    [[nodiscard]] static Prefab fromEntity(const Scene& scene, Entity root);

    /** @brief Writes this prefab using the same JSON format as scenes. */
    [[nodiscard]] core::Result<void> save(const std::filesystem::path& path) const;

    [[nodiscard]] const Scene& scene() const noexcept;

private:
    std::shared_ptr<const Scene> m_scene;
};

} // namespace vshade::scene
