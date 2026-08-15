#pragma once

#include "scene/Entity.hpp"

namespace vshade::script {

class NativeScriptSystem;

/**
 * @brief Base class for compiled C++ gameplay behavior attached to an entity.
 *
 * Derived scripts override only the lifecycle callbacks they need. The owning
 * NativeScriptSystem assigns entity() before invoking onCreate().
 */
class NativeScript {
public:
    NativeScript() = default;
    virtual ~NativeScript() = default;

    NativeScript(const NativeScript&) = delete;
    NativeScript& operator=(const NativeScript&) = delete;
    NativeScript(NativeScript&&) = delete;
    NativeScript& operator=(NativeScript&&) = delete;

    /** @brief Called once after the runtime instance is attached. */
    virtual void onCreate() {}

    /** @brief Called once per rendered frame while the binding is enabled. */
    virtual void onUpdate(float deltaTime) {
        (void)deltaTime;
    }

    /** @brief Called at the fixed simulation rate while the binding is enabled. */
    virtual void onFixedUpdate(float fixedDeltaTime) {
        (void)fixedDeltaTime;
    }

    /** @brief Called before the runtime instance is released. */
    virtual void onDestroy() {}

    /** @brief Returns the entity that owns this script instance. */
    [[nodiscard]] scene::Entity entity() const noexcept {
        return m_entity;
    }

protected:
    /** @brief Returns a mutable component belonging to the owning entity. */
    template<typename Component>
    [[nodiscard]] Component& component() {
        return m_entity.component<Component>();
    }

    /** @brief Returns a read-only component belonging to the owning entity. */
    template<typename Component>
    [[nodiscard]] const Component& component() const {
        return m_entity.component<Component>();
    }

private:
    friend class NativeScriptSystem;

    void attach(const scene::Entity entity) noexcept {
        m_entity = entity;
    }

    scene::Entity m_entity;
};

} // namespace vshade::script
