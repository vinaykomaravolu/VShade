#include "physics/physics2d/PhysicsSystem2D.hpp"

#include "math/Quaternion.hpp"
#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Scene.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>

namespace vshade::physics {

struct PhysicsSystem2D::Impl {
    explicit Impl(const PhysicsWorld2DSettings& initialSettings)
        : settings(initialSettings), physicsWorld(initialSettings) {}

    [[nodiscard]] PhysicsBody2D createBody(const scene::Entity entity) {
        const scene::TransformComponent& transformComponent =
            entity.component<scene::TransformComponent>();
        const scene::RigidBody2DComponent& rigidBody =
            entity.component<scene::RigidBody2DComponent>();
        const scene::Collider2DComponent& collider =
            entity.component<scene::Collider2DComponent>();

        PhysicsBody2DSettings bodySettings = rigidBody.settings;
        bodySettings.position = {
            transformComponent.transform.position().x,
            transformComponent.transform.position().y
        };
        bodySettings.rotationRadians =
            math::toEuler(transformComponent.transform.rotation()).z;
        bodySettings.userData = entity.uuid();
        return physicsWorld.createBody(
            bodySettings,
            collider.shape,
            collider.material,
            collider.offset,
            collider.sensor
        );
    }

    void removeMissingBodies(scene::Scene& scene) {
        for (auto iterator = bodies.begin(); iterator != bodies.end();) {
            const scene::Entity entity = scene.findEntity(iterator->first);
            if (!entity || !entity.hasComponents<
                    scene::RigidBody2DComponent,
                    scene::Collider2DComponent
                >()) {
                if (physicsWorld.contains(iterator->second)) {
                    physicsWorld.destroyBody(iterator->second);
                }
                iterator = bodies.erase(iterator);
            } else {
                ++iterator;
            }
        }
    }

    void addMissingBodies(scene::Scene& scene) {
        auto view = scene.view<
            const scene::UUIDComponent,
            const scene::RigidBody2DComponent,
            const scene::Collider2DComponent
        >();
        for (const auto [handle, uuid, rigidBody, collider] : view.each()) {
            (void)rigidBody;
            (void)collider;
            if (!bodies.contains(uuid.uuid)) {
                bodies.emplace(uuid.uuid, createBody(scene.findEntity(uuid.uuid)));
            }
            (void)handle;
        }
    }

    PhysicsWorld2DSettings settings;
    PhysicsWorld2D physicsWorld;
    std::unordered_map<std::uint64_t, PhysicsBody2D> bodies;
};

PhysicsSystem2D::PhysicsSystem2D(const PhysicsWorld2DSettings& settings)
    : m_impl(std::make_unique<Impl>(settings)) {}

PhysicsSystem2D::~PhysicsSystem2D() = default;
PhysicsSystem2D::PhysicsSystem2D(PhysicsSystem2D&&) noexcept = default;
PhysicsSystem2D& PhysicsSystem2D::operator=(PhysicsSystem2D&&) noexcept = default;

void PhysicsSystem2D::rebuild(scene::Scene& scene) {
    clear();
    m_impl->addMissingBodies(scene);
}

void PhysicsSystem2D::update(scene::Scene& scene, const float fixedDeltaTime) {
    m_impl->removeMissingBodies(scene);
    m_impl->addMissingBodies(scene);

    for (const auto& [uuid, body] : m_impl->bodies) {
        scene::Entity entity = scene.findEntity(uuid);
        const scene::RigidBody2DComponent& rigidBody =
            entity.component<scene::RigidBody2DComponent>();
        if (rigidBody.settings.type != BodyType::Dynamic) {
            const math::Transform& transform =
                entity.component<scene::TransformComponent>().transform;
            m_impl->physicsWorld.setTransform(
                body,
                {transform.position().x, transform.position().y},
                math::toEuler(transform.rotation()).z
            );
        }
    }

    m_impl->physicsWorld.step(fixedDeltaTime);

    for (const auto& [uuid, body] : m_impl->bodies) {
        scene::Entity entity = scene.findEntity(uuid);
        const scene::RigidBody2DComponent& rigidBody =
            entity.component<scene::RigidBody2DComponent>();
        if (rigidBody.settings.type == BodyType::Dynamic) {
            math::Transform& transform =
                entity.component<scene::TransformComponent>().transform;
            math::Vec3 position = transform.position();
            const math::Vec2 physicsPosition = m_impl->physicsWorld.position(body);
            position.x = physicsPosition.x;
            position.y = physicsPosition.y;
            transform.setPosition(position);
            transform.setRotation(math::fromEuler({
                0.0F,
                0.0F,
                m_impl->physicsWorld.rotation(body)
            }));
        }
    }
}

void PhysicsSystem2D::clear() {
    m_impl->bodies.clear();
    m_impl->physicsWorld = PhysicsWorld2D(m_impl->settings);
}

PhysicsWorld2D& PhysicsSystem2D::world() noexcept {
    return m_impl->physicsWorld;
}

const PhysicsWorld2D& PhysicsSystem2D::world() const noexcept {
    return m_impl->physicsWorld;
}

} // namespace vshade::physics
