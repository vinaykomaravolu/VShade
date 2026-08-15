#pragma once

#include <cstddef>
#include <memory>

namespace vshade::core { class EngineServices; }

namespace vshade::scene {

class Scene;

/** @brief Owns scene-scoped voices and synchronizes audio components. */
class SceneAudioSystem final {
public:
    explicit SceneAudioSystem(core::EngineServices& services);
    ~SceneAudioSystem();

    SceneAudioSystem(const SceneAudioSystem&) = delete;
    SceneAudioSystem& operator=(const SceneAudioSystem&) = delete;
    SceneAudioSystem(SceneAudioSystem&&) noexcept;
    SceneAudioSystem& operator=(SceneAudioSystem&&) noexcept;

    void attachScene(Scene& scene);
    void update();
    void detachScene() noexcept;

    [[nodiscard]] bool hasAttachedScene() const noexcept;
    [[nodiscard]] std::size_t voiceCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::scene
