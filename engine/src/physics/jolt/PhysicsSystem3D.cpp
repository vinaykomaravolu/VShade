#include "physics/physics3d/PhysicsSystem3D.hpp"

#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Scene.hpp"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vshade::physics {

struct PhysicsSystem3D::Impl {
    explicit Impl(const PhysicsWorld3DSettings& initialSettings)
        : settings(initialSettings), physicsWorld(initialSettings) {
        installContactCollector();
    }

    void installContactCollector() {
        physicsWorld.setContactListener([this](const ContactEvent3D& event) {
            pendingContacts.push_back(event);
        });
    }

    [[nodiscard]] scene::Entity entityFor(const BodyId3D id) const noexcept {
        if (attachedScene == nullptr) return {};
        for (const auto& [uuid, body] : bodies) {
            if (body.id() == id) return attachedScene->findEntity(uuid);
        }
        return {};
    }

    void dispatchContacts() {
        if (contactListener) {
            for (const auto& event : pendingContacts) {
                const scene::Entity first = entityFor(event.firstBody);
                const scene::Entity second = entityFor(event.secondBody);
                if (first && second) {
                    contactListener({
                        .first = first,
                        .second = second,
                        .normal = event.normal,
                        .point = event.point,
                        .phase = event.phase,
                    });
                }
            }
        }
        pendingContacts.clear();
    }

    [[nodiscard]] PhysicsBody3D createBody(const scene::Entity entity) {
        const scene::TransformComponent& transform =
            entity.component<scene::TransformComponent>();
        const scene::RigidBody3DComponent& rigidBody =
            entity.component<scene::RigidBody3DComponent>();
        const scene::Collider3DComponent& collider =
            entity.component<scene::Collider3DComponent>();

        PhysicsBody3DSettings bodySettings = rigidBody.settings;
        bodySettings.position = transform.transform.position();
        bodySettings.rotation = transform.transform.rotation();
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
                    scene::RigidBody3DComponent,
                    scene::Collider3DComponent
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
            const scene::RigidBody3DComponent,
            const scene::Collider3DComponent
        >();
        for (const auto [handle, uuid, rigidBody, collider] : view.each()) {
            (void)handle;
            (void)rigidBody;
            (void)collider;
            if (!bodies.contains(uuid.uuid)) {
                bodies.emplace(uuid.uuid, createBody(scene.findEntity(uuid.uuid)));
            }
        }
    }

    PhysicsWorld3DSettings settings;
    PhysicsWorld3D physicsWorld;
    std::unordered_map<std::uint64_t, PhysicsBody3D> bodies;
    scene::Scene* attachedScene = nullptr;
    SceneContactListener3D contactListener;
    std::vector<ContactEvent3D> pendingContacts;
};

PhysicsSystem3D::PhysicsSystem3D(const PhysicsWorld3DSettings& settings)
    : m_impl(std::make_unique<Impl>(settings)) {}

PhysicsSystem3D::~PhysicsSystem3D() = default;
PhysicsSystem3D::PhysicsSystem3D(PhysicsSystem3D&&) noexcept = default;
PhysicsSystem3D& PhysicsSystem3D::operator=(PhysicsSystem3D&&) noexcept = default;

void PhysicsSystem3D::rebuild(scene::Scene& scene) {
    clear();
    m_impl->attachedScene = &scene;
    m_impl->addMissingBodies(scene);
}

void PhysicsSystem3D::update(scene::Scene& scene, const float fixedDeltaTime) {
    m_impl->attachedScene = &scene;
    m_impl->removeMissingBodies(scene);
    m_impl->addMissingBodies(scene);

    for (const auto& [uuid, body] : m_impl->bodies) {
        scene::Entity entity = scene.findEntity(uuid);
        const scene::RigidBody3DComponent& rigidBody =
            entity.component<scene::RigidBody3DComponent>();
        if (rigidBody.settings.type != BodyType::Dynamic) {
            const math::Transform& transform =
                entity.component<scene::TransformComponent>().transform;
            m_impl->physicsWorld.setTransform(
                body,
                transform.position(),
                transform.rotation()
            );
        }
    }

    m_impl->physicsWorld.step(fixedDeltaTime);
    m_impl->dispatchContacts();

    for (const auto& [uuid, body] : m_impl->bodies) {
        scene::Entity entity = scene.findEntity(uuid);
        const scene::RigidBody3DComponent& rigidBody =
            entity.component<scene::RigidBody3DComponent>();
        if (rigidBody.settings.type == BodyType::Dynamic) {
            math::Transform& transform =
                entity.component<scene::TransformComponent>().transform;
            transform.setPosition(m_impl->physicsWorld.position(body));
            transform.setRotation(m_impl->physicsWorld.rotation(body));
        }
    }
}

void PhysicsSystem3D::clear() {
    m_impl->bodies.clear();
    m_impl->physicsWorld = PhysicsWorld3D(m_impl->settings);
    m_impl->installContactCollector();
    m_impl->pendingContacts.clear();
    m_impl->attachedScene = nullptr;
}

void PhysicsSystem3D::setContactListener(SceneContactListener3D listener) {
    m_impl->contactListener = std::move(listener);
}

std::optional<PhysicsBody3D> PhysicsSystem3D::body(
    const scene::Entity entity
) const noexcept {
    if (!entity || m_impl->attachedScene == nullptr ||
        !m_impl->attachedScene->valid(entity)) {
        return std::nullopt;
    }
    const auto found = m_impl->bodies.find(entity.uuid());
    return found == m_impl->bodies.end()
        ? std::nullopt
        : std::optional<PhysicsBody3D>{found->second};
}

void PhysicsSystem3D::applyImpulse(
    const scene::Entity entity,
    const math::Vec3& impulse
) {
    const auto physicsBody = body(entity);
    if (!physicsBody) {
        throw std::invalid_argument("Entity has no runtime 3D physics body");
    }
    m_impl->physicsWorld.applyImpulse(*physicsBody, impulse);
}

std::optional<SceneRaycastHit3D> PhysicsSystem3D::raycast(
    const RaycastQuery3D& query
) const {
    const auto hit = m_impl->physicsWorld.raycast(query);
    if (!hit || m_impl->attachedScene == nullptr) return std::nullopt;
    for (const auto& [uuid, physicsBody] : m_impl->bodies) {
        if (physicsBody.id() == hit->body) {
            return SceneRaycastHit3D{
                .entity = m_impl->attachedScene->findEntity(uuid),
                .physics = *hit,
            };
        }
    }
    return std::nullopt;
}

PhysicsWorld3D& PhysicsSystem3D::world() noexcept {
    return m_impl->physicsWorld;
}

const PhysicsWorld3D& PhysicsSystem3D::world() const noexcept {
    return m_impl->physicsWorld;
}

} // namespace vshade::physics
