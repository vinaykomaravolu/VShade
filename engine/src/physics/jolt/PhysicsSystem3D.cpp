#include "physics/physics3d/PhysicsSystem3D.hpp"

#include "asset/AssetManager.hpp"
#include "renderer/Model.hpp"
#include "scene/Components.hpp"
#include "scene/Entity.hpp"
#include "scene/Scene.hpp"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace vshade::physics {

namespace {

[[nodiscard]] bool hasCollider(const scene::Entity entity) {
    return entity.has<scene::Collider3DComponent>()
        || entity.has<scene::BoxCollider3DComponent>()
        || entity.has<scene::SphereCollider3DComponent>()
        || entity.has<scene::CapsuleCollider3DComponent>()
        || entity.has<scene::CylinderCollider3DComponent>()
        || entity.has<scene::MeshCollider3DComponent>()
        || entity.has<scene::ConvexCollider3DComponent>();
}

struct ResolvedCollider {
    PhysicsShape3D shape;
    PhysicsMaterial3D material;
    math::Vec3 offset{0.0F};
    bool sensor = false;
};

template<typename Component>
[[nodiscard]] ResolvedCollider resolvedPrimitive(
    const Component& component,
    PhysicsShape3D shape
) {
    return {
        .shape = std::move(shape),
        .material = component.material,
        .offset = component.offset,
        .sensor = component.sensor,
    };
}

[[nodiscard]] asset::AssetReference<renderer::Model> colliderModelReference(
    const scene::Entity entity,
    const asset::AssetReference<renderer::Model>& explicitModel
) {
    if (explicitModel.valid()) {
        return explicitModel;
    }
    if (const auto* renderer =
            entity.tryGet<scene::ModelRendererComponent>()) {
        return renderer->model;
    }
    return {};
}

} // namespace

struct PhysicsSystem3D::Impl {
    explicit Impl(
        const PhysicsWorld3DSettings& initialSettings,
        asset::AssetManager* initialAssets
    ) : settings(initialSettings),
        physicsWorld(initialSettings),
        assets(initialAssets) {
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
        ResolvedCollider collider;
        bool legacyCollider = false;
        if (const auto* legacy = entity.tryGet<scene::Collider3DComponent>()) {
            legacyCollider = true;
            collider = {
                .shape = legacy->shape,
                .material = legacy->material,
                .offset = legacy->offset,
                .sensor = legacy->sensor,
            };
        } else if (const auto* box =
                entity.tryGet<scene::BoxCollider3DComponent>()) {
            collider = resolvedPrimitive(
                *box,
                BoxShape3D{box->halfExtents}
            );
        } else if (const auto* sphere =
                entity.tryGet<scene::SphereCollider3DComponent>()) {
            collider = resolvedPrimitive(*sphere, SphereShape3D{sphere->radius});
        } else if (const auto* capsule =
                entity.tryGet<scene::CapsuleCollider3DComponent>()) {
            collider = resolvedPrimitive(
                *capsule,
                CapsuleShape3D{capsule->halfHeight, capsule->radius}
            );
        } else if (const auto* cylinder =
                entity.tryGet<scene::CylinderCollider3DComponent>()) {
            collider = resolvedPrimitive(
                *cylinder,
                CylinderShape3D{cylinder->halfHeight, cylinder->radius}
            );
        } else {
            const auto* mesh = entity.tryGet<scene::MeshCollider3DComponent>();
            const auto* convex =
                entity.tryGet<scene::ConvexCollider3DComponent>();
            const auto modelReference = colliderModelReference(
                entity,
                mesh ? mesh->model : convex->model
            );
            if (assets == nullptr || !modelReference.valid()) {
                throw std::invalid_argument(
                    "Mesh and convex colliders require a resolvable model asset"
                );
            }
            const auto model = assets->loadResource(modelReference).shared();
            if (model->collisionVertices().empty()) {
                throw std::invalid_argument(
                    "Collider model contains no triangle geometry"
                );
            }
            if (mesh) {
                collider = resolvedPrimitive(
                    *mesh,
                    MeshShape3D{
                        model->collisionVertices(),
                        model->collisionIndices(),
                    }
                );
            } else {
                collider = resolvedPrimitive(
                    *convex,
                    ConvexShape3D{model->collisionVertices()}
                );
            }
        }

        if (!legacyCollider) {
            const math::Vec3 scale = glm::abs(transform.transform.scale());
            collider.offset *= scale;
            std::visit(
                [&scale](auto& shape) {
                    using Shape = std::remove_cvref_t<decltype(shape)>;
                    if constexpr (std::same_as<Shape, BoxShape3D>) {
                        shape.halfExtents *= scale;
                    } else if constexpr (std::same_as<Shape, SphereShape3D>) {
                        shape.radius *= std::max({scale.x, scale.y, scale.z});
                    } else if constexpr (
                        std::same_as<Shape, CapsuleShape3D>
                        || std::same_as<Shape, CylinderShape3D>
                    ) {
                        shape.halfHeight *= scale.y;
                        shape.radius *= std::max(scale.x, scale.z);
                    } else if constexpr (std::same_as<Shape, MeshShape3D>) {
                        for (math::Vec3& vertex : shape.vertices) {
                            vertex *= scale;
                        }
                    } else if constexpr (std::same_as<Shape, ConvexShape3D>) {
                        for (math::Vec3& point : shape.points) {
                            point *= scale;
                        }
                    }
                },
                collider.shape
            );
        }

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
            if (!entity
                || !entity.has<scene::RigidBody3DComponent>()
                || !hasCollider(entity)) {
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
            const scene::RigidBody3DComponent
        >();
        for (const auto [handle, uuid, rigidBody] : view.each()) {
            (void)handle;
            (void)rigidBody;
            const scene::Entity entity = scene.findEntity(uuid.uuid);
            if (hasCollider(entity) && !bodies.contains(uuid.uuid)) {
                bodies.emplace(uuid.uuid, createBody(entity));
            }
        }
    }

    PhysicsWorld3DSettings settings;
    PhysicsWorld3D physicsWorld;
    asset::AssetManager* assets = nullptr;
    std::unordered_map<std::uint64_t, PhysicsBody3D> bodies;
    scene::Scene* attachedScene = nullptr;
    SceneContactListener3D contactListener;
    std::vector<ContactEvent3D> pendingContacts;
};

PhysicsSystem3D::PhysicsSystem3D(
    const PhysicsWorld3DSettings& settings,
    asset::AssetManager* assets
) : m_impl(std::make_unique<Impl>(settings, assets)) {}

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
