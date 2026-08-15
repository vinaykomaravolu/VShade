#pragma once

#include <cstddef>
#include <memory>

namespace vshade::scene {
class Entity;
class Scene;
}

namespace vshade::script {

class NativeScriptRegistry;
class NativeScript;

/**
 * @brief Owns native C++ script instances and dispatches their lifecycle.
 *
 * Only scene::ScriptBinding entries using ScriptBackend::NativeCpp are consumed.
 * Future language backends can implement parallel systems over the same
 * scene::ScriptComponent.
 */
class NativeScriptSystem final {
public:
    explicit NativeScriptSystem(NativeScriptRegistry& registry);
    ~NativeScriptSystem();

    NativeScriptSystem(const NativeScriptSystem&) = delete;
    NativeScriptSystem& operator=(const NativeScriptSystem&) = delete;
    NativeScriptSystem(NativeScriptSystem&&) noexcept;
    NativeScriptSystem& operator=(NativeScriptSystem&&) noexcept;

    /**
     * @brief Attaches to @p scene and creates its existing native script instances.
     * @note The scene must outlive this attachment or be detached first.
     * @throws std::logic_error If a scene is already attached.
     */
    void attachScene(scene::Scene& scene);

    /**
     * @brief Synchronizes bindings and invokes onUpdate() on enabled instances.
     * Newly added bindings receive onCreate() before their first update.
     * @throws std::logic_error If no scene is attached.
     */
    void update(float deltaTime);

    /**
     * @brief Invokes onFixedUpdate() on enabled native instances.
     * @throws std::logic_error If no scene is attached.
     */
    void fixedUpdate(float fixedDeltaTime);

    /**
     * @brief Invokes onDestroy(), releases all instances, and detaches the scene.
     */
    void detachScene() noexcept;

    /** @brief Reports whether this system currently has an attached scene. */
    [[nodiscard]] bool hasAttachedScene() const noexcept;

    /** @brief Returns the number of live native C++ script instances. */
    [[nodiscard]] std::size_t instanceCount() const noexcept;

private:
    static void attachInstance(NativeScript& script, scene::Entity entity) noexcept;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vshade::script
