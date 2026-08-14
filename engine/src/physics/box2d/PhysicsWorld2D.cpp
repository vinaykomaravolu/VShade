#include "physics/physics2d/PhysicsWorld2D.hpp"

#include <box2d/box2d.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace vshade::physics {

namespace {

[[nodiscard]] b2Vec2 toBoxVector(const math::Vec2& vector) noexcept {
    return {vector.x, vector.y};
}

[[nodiscard]] b2Pos toBoxPosition(const math::Vec2& position) noexcept {
    return {position.x, position.y};
}

[[nodiscard]] math::Vec2 fromBoxVector(const b2Vec2 vector) noexcept {
    return {vector.x, vector.y};
}

[[nodiscard]] math::Vec2 fromBoxPosition(const b2Pos position) noexcept {
    return {
        static_cast<float>(position.x),
        static_cast<float>(position.y)
    };
}

[[nodiscard]] bool finite(const math::Vec2& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

void validateBodySettings(const PhysicsBody2DSettings& settings) {
    if (!finite(settings.position) || !finite(settings.linearVelocity) ||
        !std::isfinite(settings.rotationRadians) ||
        !std::isfinite(settings.angularVelocity) ||
        !std::isfinite(settings.linearDamping) || settings.linearDamping < 0.0F ||
        !std::isfinite(settings.angularDamping) || settings.angularDamping < 0.0F ||
        !std::isfinite(settings.gravityScale)) {
        throw std::invalid_argument("Physics body settings must contain finite values and non-negative damping");
    }
}

void validateMaterial(const PhysicsMaterial2D& material) {
    if (!std::isfinite(material.density) || material.density < 0.0F ||
        !std::isfinite(material.friction) || material.friction < 0.0F ||
        !std::isfinite(material.restitution) || material.restitution < 0.0F) {
        throw std::invalid_argument("Physics material values must be finite and non-negative");
    }
}

void validateShape(const PhysicsShape2D& shape) {
    std::visit(
        [](const auto& typedShape) {
            using Shape = std::remove_cvref_t<decltype(typedShape)>;
            if constexpr (std::same_as<Shape, BoxShape2D>) {
                if (!finite(typedShape.halfExtents) ||
                    typedShape.halfExtents.x <= 0.0F ||
                    typedShape.halfExtents.y <= 0.0F) {
                    throw std::invalid_argument("Box half extents must be finite and positive");
                }
            } else if constexpr (std::same_as<Shape, CircleShape2D>) {
                if (!std::isfinite(typedShape.radius) || typedShape.radius <= 0.0F) {
                    throw std::invalid_argument("Circle radius must be finite and positive");
                }
            } else if constexpr (std::same_as<Shape, CapsuleShape2D>) {
                if (!std::isfinite(typedShape.halfHeight) || typedShape.halfHeight < 0.0F ||
                    !std::isfinite(typedShape.radius) || typedShape.radius <= 0.0F) {
                    throw std::invalid_argument("Capsule dimensions must be finite and positive");
                }
            }
        },
        shape
    );
}

[[nodiscard]] b2BodyType toBoxBodyType(const BodyType type) noexcept {
    switch (type) {
    case BodyType::Static:
        return b2_staticBody;
    case BodyType::Dynamic:
        return b2_dynamicBody;
    case BodyType::Kinematic:
        return b2_kinematicBody;
    }
    return b2_staticBody;
}

[[nodiscard]] std::uint32_t nextWorldToken() noexcept {
    static std::atomic<std::uint32_t> next{1};
    std::uint32_t token = next.fetch_add(1, std::memory_order_relaxed);
    if (token == 0) {
        token = next.fetch_add(1, std::memory_order_relaxed);
    }
    return token;
}

} // namespace

struct PhysicsWorld2D::Impl {
    struct BodySlot {
        b2BodyId body = b2_nullBodyId;
        b2ShapeId shape = b2_nullShapeId;
        std::uint32_t generation = 1;
        bool active = false;
    };

    struct ActiveContact {
        b2ContactId contact = b2_nullContactId;
        BodyId2D firstBody;
        BodyId2D secondBody;
    };

    explicit Impl(const PhysicsWorld2DSettings& settings)
        : worldToken(nextWorldToken()) {
        if (!finite(settings.gravity)) {
            throw std::invalid_argument("Physics world gravity must be finite");
        }
        b2WorldDef definition = b2DefaultWorldDef();
        definition.gravity = toBoxVector(settings.gravity);
        world = b2CreateWorld(&definition);
        if (!b2World_IsValid(world)) {
            throw std::runtime_error("Box2D failed to create a physics world");
        }
    }

    ~Impl() {
        if (b2World_IsValid(world)) {
            b2DestroyWorld(world);
        }
    }

    [[nodiscard]] bool contains(const PhysicsBody2D handle) const noexcept {
        const BodyId2D id = handle.id();
        if (id.world != worldToken || id.index == 0 || id.index > bodies.size()) {
            return false;
        }
        const BodySlot& slot = bodies[id.index - 1];
        return slot.active && slot.generation == id.generation && b2Body_IsValid(slot.body);
    }

    [[nodiscard]] BodySlot& require(const PhysicsBody2D handle) {
        if (!contains(handle)) {
            throw std::invalid_argument("Physics body handle is invalid or no longer alive");
        }
        return bodies[handle.id().index - 1];
    }

    [[nodiscard]] const BodySlot& require(const PhysicsBody2D handle) const {
        if (!contains(handle)) {
            throw std::invalid_argument("Physics body handle is invalid or no longer alive");
        }
        return bodies[handle.id().index - 1];
    }

    [[nodiscard]] BodyId2D handleForShape(const b2ShapeId shape) const noexcept {
        const auto found = shapeBodies.find(b2StoreShapeId(shape));
        return found == shapeBodies.end() ? BodyId2D{} : found->second;
    }

    [[nodiscard]] ContactEvent2D makeContactEvent(
        const ActiveContact& active,
        const ContactPhase phase
    ) const {
        ContactEvent2D event{
            .firstBody = active.firstBody,
            .secondBody = active.secondBody,
            .phase = phase,
        };
        if (b2Contact_IsValid(active.contact)) {
            const b2ContactData data = b2Contact_GetData(active.contact);
            event.normal = fromBoxVector(data.manifold.normal);
            if (data.manifold.pointCount > 0 && b2Shape_IsValid(data.shapeIdA)) {
                const b2BodyId body = b2Shape_GetBody(data.shapeIdA);
                const b2Pos point = b2OffsetPos(
                    b2Body_GetWorldCenter(body),
                    data.manifold.points[0].anchorA
                );
                event.point = fromBoxPosition(point);
            }
        }
        return event;
    }

    void dispatchEvents() {
        if (!contactListener) {
            return;
        }

        const b2ContactEvents contacts = b2World_GetContactEvents(world);
        std::vector<ContactEvent2D> pending;
        pending.reserve(
            activeContacts.size() +
            static_cast<std::size_t>(contacts.beginCount + contacts.endCount)
        );

        for (const ActiveContact& active : activeContacts) {
            bool ended = false;
            for (int index = 0; index < contacts.endCount; ++index) {
                if (B2_ID_EQUALS(active.contact, contacts.endEvents[index].contactId)) {
                    ended = true;
                    break;
                }
            }
            if (!ended && b2Contact_IsValid(active.contact)) {
                pending.push_back(makeContactEvent(active, ContactPhase::Persisted));
            }
        }

        for (int index = 0; index < contacts.beginCount; ++index) {
            const b2ContactBeginTouchEvent& event = contacts.beginEvents[index];
            ActiveContact active{
                .contact = event.contactId,
                .firstBody = handleForShape(event.shapeIdA),
                .secondBody = handleForShape(event.shapeIdB),
            };
            if (active.firstBody && active.secondBody) {
                activeContacts.push_back(active);
                pending.push_back(makeContactEvent(active, ContactPhase::Began));
            }
        }

        for (int index = 0; index < contacts.endCount; ++index) {
            const b2ContactEndTouchEvent& event = contacts.endEvents[index];
            const auto found = std::find_if(
                activeContacts.begin(),
                activeContacts.end(),
                [&event](const ActiveContact& active) {
                    return B2_ID_EQUALS(active.contact, event.contactId);
                }
            );
            if (found != activeContacts.end()) {
                pending.push_back(makeContactEvent(*found, ContactPhase::Ended));
                activeContacts.erase(found);
            }
        }

        const b2SensorEvents sensors = b2World_GetSensorEvents(world);
        for (int index = 0; index < sensors.beginCount; ++index) {
            const b2SensorBeginTouchEvent& event = sensors.beginEvents[index];
            const BodyId2D sensor = handleForShape(event.sensorShapeId);
            const BodyId2D visitor = handleForShape(event.visitorShapeId);
            if (sensor && visitor) {
                pending.push_back({
                    .firstBody = sensor,
                    .secondBody = visitor,
                    .phase = ContactPhase::Began,
                });
            }
        }
        for (int index = 0; index < sensors.endCount; ++index) {
            const b2SensorEndTouchEvent& event = sensors.endEvents[index];
            const BodyId2D sensor = handleForShape(event.sensorShapeId);
            const BodyId2D visitor = handleForShape(event.visitorShapeId);
            if (sensor && visitor) {
                pending.push_back({
                    .firstBody = sensor,
                    .secondBody = visitor,
                    .phase = ContactPhase::Ended,
                });
            }
        }

        for (const ContactEvent2D& event : pending) {
            contactListener(event);
        }
    }

    b2WorldId world = b2_nullWorldId;
    std::uint32_t worldToken = 0;
    std::vector<BodySlot> bodies;
    std::vector<std::uint32_t> freeSlots;
    std::unordered_map<std::uint64_t, BodyId2D> shapeBodies;
    std::vector<ActiveContact> activeContacts;
    ContactListener2D contactListener;
};

PhysicsWorld2D::PhysicsWorld2D(const PhysicsWorld2DSettings& settings)
    : m_impl(std::make_unique<Impl>(settings)) {}

PhysicsWorld2D::~PhysicsWorld2D() = default;
PhysicsWorld2D::PhysicsWorld2D(PhysicsWorld2D&&) noexcept = default;
PhysicsWorld2D& PhysicsWorld2D::operator=(PhysicsWorld2D&&) noexcept = default;

PhysicsBody2D PhysicsWorld2D::createBody(
    const PhysicsBody2DSettings& settings,
    const PhysicsShape2D& shape,
    const PhysicsMaterial2D& material,
    const math::Vec2& colliderOffset,
    const bool sensor
) {
    validateBodySettings(settings);
    validateShape(shape);
    validateMaterial(material);
    if (!finite(colliderOffset)) {
        throw std::invalid_argument("Collider offset must be finite");
    }

    b2BodyDef bodyDefinition = b2DefaultBodyDef();
    bodyDefinition.type = toBoxBodyType(settings.type);
    bodyDefinition.position = toBoxPosition(settings.position);
    bodyDefinition.rotation = b2MakeRot(settings.rotationRadians);
    bodyDefinition.linearVelocity = toBoxVector(settings.linearVelocity);
    bodyDefinition.angularVelocity = settings.angularVelocity;
    bodyDefinition.linearDamping = settings.linearDamping;
    bodyDefinition.angularDamping = settings.angularDamping;
    bodyDefinition.gravityScale = settings.gravityScale;
    bodyDefinition.motionLocks.angularZ = settings.fixedRotation;
    bodyDefinition.isBullet = settings.continuousCollision;
    bodyDefinition.isEnabled = settings.enabled;

    const b2BodyId nativeBody = b2CreateBody(m_impl->world, &bodyDefinition);
    b2ShapeDef shapeDefinition = b2DefaultShapeDef();
    shapeDefinition.density = material.density;
    shapeDefinition.material.friction = material.friction;
    shapeDefinition.material.restitution = material.restitution;
    shapeDefinition.filter.categoryBits = settings.collision.layer;
    shapeDefinition.filter.maskBits = settings.collision.mask;
    shapeDefinition.isSensor = sensor;
    shapeDefinition.enableSensorEvents = true;
    shapeDefinition.enableContactEvents = true;

    b2ShapeId nativeShape = b2_nullShapeId;
    std::visit(
        [&](const auto& typedShape) {
            using Shape = std::remove_cvref_t<decltype(typedShape)>;
            if constexpr (std::same_as<Shape, BoxShape2D>) {
                const b2Polygon box = b2MakeOffsetBox(
                    typedShape.halfExtents.x,
                    typedShape.halfExtents.y,
                    toBoxVector(colliderOffset),
                    b2MakeRot(0.0F)
                );
                nativeShape = b2CreatePolygonShape(nativeBody, &shapeDefinition, &box);
            } else if constexpr (std::same_as<Shape, CircleShape2D>) {
                const b2Circle circle{
                    .center = toBoxVector(colliderOffset),
                    .radius = typedShape.radius,
                };
                nativeShape = b2CreateCircleShape(nativeBody, &shapeDefinition, &circle);
            } else if constexpr (std::same_as<Shape, CapsuleShape2D>) {
                const b2Capsule capsule{
                    .center1 = toBoxVector(colliderOffset + math::Vec2{0.0F, -typedShape.halfHeight}),
                    .center2 = toBoxVector(colliderOffset + math::Vec2{0.0F, typedShape.halfHeight}),
                    .radius = typedShape.radius,
                };
                nativeShape = b2CreateCapsuleShape(nativeBody, &shapeDefinition, &capsule);
            }
        },
        shape
    );
    if (!b2Shape_IsValid(nativeShape)) {
        b2DestroyBody(nativeBody);
        throw std::runtime_error("Box2D failed to create a collider shape");
    }

    std::uint32_t slotIndex = 0;
    if (m_impl->freeSlots.empty()) {
        slotIndex = static_cast<std::uint32_t>(m_impl->bodies.size());
        m_impl->bodies.emplace_back();
    } else {
        slotIndex = m_impl->freeSlots.back();
        m_impl->freeSlots.pop_back();
    }

    Impl::BodySlot& slot = m_impl->bodies[slotIndex];
    slot.body = nativeBody;
    slot.shape = nativeShape;
    slot.active = true;
    const BodyId2D id{m_impl->worldToken, slotIndex + 1, slot.generation};
    m_impl->shapeBodies.emplace(b2StoreShapeId(nativeShape), id);
    return PhysicsBody2D(id);
}

void PhysicsWorld2D::destroyBody(const PhysicsBody2D body) {
    Impl::BodySlot& slot = m_impl->require(body);
    m_impl->shapeBodies.erase(b2StoreShapeId(slot.shape));
    m_impl->activeContacts.erase(
        std::remove_if(
            m_impl->activeContacts.begin(),
            m_impl->activeContacts.end(),
            [id = body.id()](const Impl::ActiveContact& active) {
                return active.firstBody == id || active.secondBody == id;
            }
        ),
        m_impl->activeContacts.end()
    );
    b2DestroyBody(slot.body);
    slot.body = b2_nullBodyId;
    slot.shape = b2_nullShapeId;
    slot.active = false;
    ++slot.generation;
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    m_impl->freeSlots.push_back(body.id().index - 1);
}

bool PhysicsWorld2D::contains(const PhysicsBody2D body) const noexcept {
    return m_impl != nullptr && m_impl->contains(body);
}

void PhysicsWorld2D::step(const float fixedDeltaTime) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0F) {
        throw std::invalid_argument("Physics time step must be finite and positive");
    }
    b2World_Step(m_impl->world, fixedDeltaTime, 4);
    m_impl->dispatchEvents();
}

void PhysicsWorld2D::setGravity(const math::Vec2& gravity) {
    if (!finite(gravity)) {
        throw std::invalid_argument("Physics world gravity must be finite");
    }
    b2World_SetGravity(m_impl->world, toBoxVector(gravity));
}

math::Vec2 PhysicsWorld2D::gravity() const {
    return fromBoxVector(b2World_GetGravity(m_impl->world));
}

void PhysicsWorld2D::setTransform(
    const PhysicsBody2D body,
    const math::Vec2& position,
    const float rotationRadians
) {
    if (!finite(position) || !std::isfinite(rotationRadians)) {
        throw std::invalid_argument("Physics transform must contain finite values");
    }
    b2Body_SetTransform(
        m_impl->require(body).body,
        toBoxPosition(position),
        b2MakeRot(rotationRadians)
    );
}

math::Vec2 PhysicsWorld2D::position(const PhysicsBody2D body) const {
    return fromBoxPosition(b2Body_GetPosition(m_impl->require(body).body));
}

float PhysicsWorld2D::rotation(const PhysicsBody2D body) const {
    return b2Rot_GetAngle(b2Body_GetRotation(m_impl->require(body).body));
}

void PhysicsWorld2D::setLinearVelocity(
    const PhysicsBody2D body,
    const math::Vec2& velocity
) {
    if (!finite(velocity)) {
        throw std::invalid_argument("Physics velocity must be finite");
    }
    b2Body_SetLinearVelocity(m_impl->require(body).body, toBoxVector(velocity));
}

math::Vec2 PhysicsWorld2D::linearVelocity(const PhysicsBody2D body) const {
    return fromBoxVector(b2Body_GetLinearVelocity(m_impl->require(body).body));
}

void PhysicsWorld2D::applyForce(const PhysicsBody2D body, const math::Vec2& force) {
    if (!finite(force)) {
        throw std::invalid_argument("Physics force must be finite");
    }
    b2Body_ApplyForceToCenter(m_impl->require(body).body, toBoxVector(force), true);
}

void PhysicsWorld2D::applyImpulse(const PhysicsBody2D body, const math::Vec2& impulse) {
    if (!finite(impulse)) {
        throw std::invalid_argument("Physics impulse must be finite");
    }
    b2Body_ApplyLinearImpulseToCenter(m_impl->require(body).body, toBoxVector(impulse), true);
}

std::optional<RaycastHit2D> PhysicsWorld2D::raycast(const RaycastQuery2D& query) const {
    if (!finite(query.origin) || !finite(query.displacement)) {
        throw std::invalid_argument("Physics ray must contain finite values");
    }
    if (query.displacement.x == 0.0F && query.displacement.y == 0.0F) {
        return std::nullopt;
    }
    b2QueryFilter filter = b2DefaultQueryFilter();
    filter.categoryBits = query.filter.layer;
    filter.maskBits = query.filter.mask;
    const b2RayResult result = b2World_CastRayClosest(
        m_impl->world,
        toBoxPosition(query.origin),
        toBoxVector(query.displacement),
        filter
    );
    if (!result.hit) {
        return std::nullopt;
    }
    const BodyId2D body = m_impl->handleForShape(result.shapeId);
    if (!body) {
        return std::nullopt;
    }
    return RaycastHit2D{
        .body = body,
        .point = fromBoxPosition(result.point),
        .normal = fromBoxVector(result.normal),
        .fraction = result.fraction,
    };
}

void PhysicsWorld2D::setContactListener(ContactListener2D listener) {
    m_impl->contactListener = std::move(listener);
    if (!m_impl->contactListener) {
        m_impl->activeContacts.clear();
    }
}

} // namespace vshade::physics
