#pragma once

#include <filesystem>

namespace vshade::scene {

class Scene;

/**
 * @brief Future file serializer for Scene data.
 *
 * Only the interface is declared for now; the scene file format and method
 * definitions will be added when serialization work begins.
 */
class SceneSerializer final {
public:
    explicit SceneSerializer(Scene& scene) noexcept;

    /**
     * @brief Writes the attached scene to a file.
     * @throws std::logic_error Until the scene file format is implemented.
     */
    [[nodiscard]] bool serialize(const std::filesystem::path& path) const;

    /**
     * @brief Replaces the attached scene with data loaded from a file.
     * @throws std::logic_error Until the scene file format is implemented.
     */
    [[nodiscard]] bool deserialize(const std::filesystem::path& path);

private:
    Scene& m_scene;
};

} // namespace vshade::scene
