#pragma once

#include <memory>
#include <cstdint>
#include <functional>

namespace vshade::physics {
class PhysicsSystem2D;
class PhysicsSystem3D;
}

namespace vshade::core { class EngineServices; }

namespace vshade::script {
class NativeScriptRegistry;
class NativeScriptSystem;
}

namespace vshade::scene {

class Scene;
class SceneAudioSystem;
class SceneRenderer;

/** @brief Selects built-in systems installed for one playing scene. */
struct SceneRuntimeConfig {
    bool physics2D = true;
    bool physics3D = true;
    bool rendering = true;
    bool audio = true;
};

/** @brief Stable extension points around the default scene systems. */
enum class SceneRuntimePhase {
    BeforeUpdate,
    AfterUpdate,
    BeforePhysics,
    AfterPhysics,
    BeforeRender,
    AfterRender,
};

using SceneSystemCallback = std::function<void(Scene&, float)>;

/**
 * @brief Coordinates the gameplay systems executing one active scene.
 *
 * Physics bodies are created before native script instances so onCreate() can
 * observe a ready simulation. Fixed script callbacks run before physics steps,
 * allowing gameplay code to affect the current simulation tick.
 */
class SceneRuntime final {
public:
    explicit SceneRuntime(script::NativeScriptRegistry& scriptRegistry);
    SceneRuntime(core::EngineServices& services, SceneRuntimeConfig config = {});
    ~SceneRuntime();

    SceneRuntime(const SceneRuntime&) = delete;
    SceneRuntime& operator=(const SceneRuntime&) = delete;
    SceneRuntime(SceneRuntime&&) noexcept;
    SceneRuntime& operator=(SceneRuntime&&) noexcept;

    /**
     * @brief Begins executing @p scene and initializes its gameplay systems.
     * @note The scene must outlive play mode or be stopped before destruction.
     * @throws std::logic_error If another scene is already playing.
     */
    void play(Scene& scene);

    /**
     * @brief Dispatches variable-rate gameplay updates.
     * @throws std::logic_error If no scene is playing.
     */
    void update(float deltaTime);

    /**
     * @brief Dispatches fixed script updates followed by 2D and 3D physics.
     * @throws std::logic_error If no scene is playing.
     */
    void fixedUpdate(float fixedDeltaTime);

    /** @brief Renders the active scene through its highest-priority camera. */
    bool render(std::uint32_t viewportWidth, std::uint32_t viewportHeight);

    /** @brief Installs a user system at a deterministic runtime phase. */
    void addSystem(SceneRuntimePhase phase, SceneSystemCallback callback);

    /** @brief Stops gameplay systems and releases the active scene reference. */
    void stop() noexcept;

    /** @brief Suspends or resumes automatic runtime updates while preserving state. */
    void setPaused(bool paused) noexcept;

    /** @brief Reports whether automatic runtime updates are suspended. */
    [[nodiscard]] bool isPaused() const noexcept;

    /** @brief Reports whether a scene is currently executing. */
    [[nodiscard]] bool isPlaying() const noexcept;

    /** @brief Returns the active scene, or null when stopped. */
    [[nodiscard]] Scene* scene() noexcept;

    /** @brief Returns the active scene, or null when stopped. */
    [[nodiscard]] const Scene* scene() const noexcept;

    /** @brief Exposes the native scripting system owned by this runtime. */
    [[nodiscard]] script::NativeScriptSystem& nativeScripts() noexcept;

    /** @brief Exposes the native scripting system owned by this runtime. */
    [[nodiscard]] const script::NativeScriptSystem& nativeScripts() const noexcept;

    /** @brief Exposes the 2D physics system owned by this runtime. */
    [[nodiscard]] physics::PhysicsSystem2D& physics2D() noexcept;

    /** @brief Exposes the 2D physics system owned by this runtime. */
    [[nodiscard]] const physics::PhysicsSystem2D& physics2D() const noexcept;

    /** @brief Exposes the 3D physics system owned by this runtime. */
    [[nodiscard]] physics::PhysicsSystem3D& physics3D() noexcept;

    /** @brief Exposes the 3D physics system owned by this runtime. */
    [[nodiscard]] const physics::PhysicsSystem3D& physics3D() const noexcept;

    /** @brief Returns the installed scene renderer, or null in a headless runtime. */
    [[nodiscard]] SceneRenderer* sceneRenderer() noexcept;

    /** @brief Returns the installed scene audio system, or null when disabled. */
    [[nodiscard]] SceneAudioSystem* sceneAudio() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::scene
