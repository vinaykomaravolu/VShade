#pragma once

#include "core/Result.hpp"

#include <filesystem>
#include <string>

namespace vshade::scene {

class Scene;

/** @brief Controls whitespace in serialized JSON scene files. */
enum class SceneJsonFormat {
    /** @brief Human-readable JSON with indentation and line breaks. */
    Pretty,
    /** @brief Minified JSON without optional whitespace. */
    Compact,
};

/**
 * @brief Saves and loads built-in and registered scene components as readable JSON.
 */
class SceneSerializer final {
public:
    explicit SceneSerializer(Scene& scene) noexcept;
    explicit SceneSerializer(const Scene& scene) noexcept;

    /**
     * @brief Writes the attached scene to a file.
     * @param format Pretty by default; use Compact for minified JSON.
     * @return True on success; false if the file cannot be written.
     */
    [[nodiscard]] bool serialize(
        const std::filesystem::path& path,
        SceneJsonFormat format = SceneJsonFormat::Pretty
    ) const;

    /**
     * @brief Replaces the attached scene with data loaded from a file.
     * @return True on success; false for missing, malformed, or unsupported data.
     * @note The attached scene is unchanged when loading fails.
     */
    [[nodiscard]] bool deserialize(const std::filesystem::path& path);

    /** @brief Structured equivalent of serialize() for editor and tooling code. */
    [[nodiscard]] core::Result<void> serializeResult(
        const std::filesystem::path& path,
        SceneJsonFormat format = SceneJsonFormat::Pretty
    ) const;

    /** @brief Structured equivalent of deserialize() with a stable error code. */
    [[nodiscard]] core::Result<void> deserializeResult(
        const std::filesystem::path& path
    );

    /** @brief Returns the diagnostic from the most recent failed operation. */
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    void setError(std::string error) const;

    const Scene& m_scene;
    Scene* m_writableScene = nullptr;
    mutable std::string m_lastError;
};

} // namespace vshade::scene
