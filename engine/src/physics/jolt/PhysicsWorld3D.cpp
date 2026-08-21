#include "physics/physics3d/PhysicsWorld3D.hpp"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/EPhysicsUpdateError.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace vshade::physics {

namespace {

namespace ObjectLayers {
constexpr JPH::ObjectLayer nonMoving = 0;
constexpr JPH::ObjectLayer moving = 1;
constexpr JPH::ObjectLayer count = 2;
} // namespace ObjectLayers

namespace BroadPhaseLayers {
const JPH::BroadPhaseLayer nonMoving{0};
const JPH::BroadPhaseLayer moving{1};
constexpr JPH::uint count = 2;
} // namespace BroadPhaseLayers

class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::count;
    }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(
        const JPH::ObjectLayer layer
    ) const override {
        return layer == ObjectLayers::nonMoving
            ? BroadPhaseLayers::nonMoving
            : BroadPhaseLayers::moving;
    }
};

class ObjectVsBroadPhaseFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    [[nodiscard]] bool ShouldCollide(
        const JPH::ObjectLayer objectLayer,
        const JPH::BroadPhaseLayer broadPhaseLayer
    ) const override {
        return objectLayer == ObjectLayers::moving ||
            broadPhaseLayer == BroadPhaseLayers::moving;
    }
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
    [[nodiscard]] bool ShouldCollide(
        const JPH::ObjectLayer first,
        const JPH::ObjectLayer second
    ) const override {
        return first == ObjectLayers::moving || second == ObjectLayers::moving;
    }
};

class JoltRuntime final {
public:
    JoltRuntime() {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    ~JoltRuntime() {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;
};

void ensureJoltRuntime() {
    static JoltRuntime runtime;
    (void)runtime;
}

[[nodiscard]] std::uint32_t nextWorldToken() noexcept {
    static std::atomic<std::uint32_t> next{1};
    std::uint32_t token = next.fetch_add(1, std::memory_order_relaxed);
    if (token == 0) {
        token = next.fetch_add(1, std::memory_order_relaxed);
    }
    return token;
}

[[nodiscard]] bool finite(const math::Vec3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool finite(const math::Quat& value) noexcept {
    return std::isfinite(value.w) && std::isfinite(value.x) &&
        std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] JPH::Vec3 toJoltVector(const math::Vec3& vector) noexcept {
    return {vector.x, vector.y, vector.z};
}

[[nodiscard]] JPH::RVec3 toJoltPosition(const math::Vec3& position) noexcept {
    return {position.x, position.y, position.z};
}

[[nodiscard]] math::Vec3 fromJoltVector(const JPH::Vec3Arg vector) noexcept {
    return {vector.GetX(), vector.GetY(), vector.GetZ()};
}

[[nodiscard]] math::Vec3 fromJoltPosition(const JPH::RVec3Arg position) noexcept {
    return {
        static_cast<float>(position.GetX()),
        static_cast<float>(position.GetY()),
        static_cast<float>(position.GetZ())
    };
}

[[nodiscard]] JPH::Quat toJoltRotation(const math::Quat& rotation) {
    if (!finite(rotation)) {
        throw std::invalid_argument("Physics rotation must be finite");
    }
    const float lengthSquared = rotation.w * rotation.w + rotation.x * rotation.x +
        rotation.y * rotation.y + rotation.z * rotation.z;
    if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0000001F) {
        throw std::invalid_argument("Physics rotation must have non-zero length");
    }
    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    return {
        rotation.x * inverseLength,
        rotation.y * inverseLength,
        rotation.z * inverseLength,
        rotation.w * inverseLength
    };
}

[[nodiscard]] math::Quat fromJoltRotation(const JPH::QuatArg rotation) noexcept {
    return {
        rotation.GetW(),
        rotation.GetX(),
        rotation.GetY(),
        rotation.GetZ()
    };
}

[[nodiscard]] JPH::EMotionType toJoltMotionType(const BodyType type) noexcept {
    switch (type) {
    case BodyType::Static:
        return JPH::EMotionType::Static;
    case BodyType::Dynamic:
        return JPH::EMotionType::Dynamic;
    case BodyType::Kinematic:
        return JPH::EMotionType::Kinematic;
    }
    return JPH::EMotionType::Static;
}

[[nodiscard]] JPH::ObjectLayer toJoltObjectLayer(const BodyType type) noexcept {
    return type == BodyType::Static ? ObjectLayers::nonMoving : ObjectLayers::moving;
}

void validateBodySettings(const PhysicsBody3DSettings& settings) {
    if (!finite(settings.position) || !finite(settings.linearVelocity) ||
        !finite(settings.angularVelocity) || !finite(settings.rotation) ||
        !std::isfinite(settings.mass) || settings.mass <= 0.0F ||
        !std::isfinite(settings.linearDamping) || settings.linearDamping < 0.0F ||
        !std::isfinite(settings.angularDamping) || settings.angularDamping < 0.0F ||
        !std::isfinite(settings.gravityScale)) {
        throw std::invalid_argument("Physics body settings contain an invalid value");
    }
    (void)toJoltRotation(settings.rotation);
}

void validateMaterial(const PhysicsMaterial3D& material) {
    if (!std::isfinite(material.friction) || material.friction < 0.0F ||
        !std::isfinite(material.restitution) || material.restitution < 0.0F) {
        throw std::invalid_argument("Physics material values must be finite and non-negative");
    }
}

void validateShape(const PhysicsShape3D& shape) {
    std::visit(
        [](const auto& typedShape) {
            using Shape = std::remove_cvref_t<decltype(typedShape)>;
            if constexpr (std::same_as<Shape, BoxShape3D>) {
                if (!finite(typedShape.halfExtents) ||
                    typedShape.halfExtents.x <= 0.0F ||
                    typedShape.halfExtents.y <= 0.0F ||
                    typedShape.halfExtents.z <= 0.0F) {
                    throw std::invalid_argument("Box half extents must be finite and positive");
                }
            } else if constexpr (std::same_as<Shape, SphereShape3D>) {
                if (!std::isfinite(typedShape.radius) || typedShape.radius <= 0.0F) {
                    throw std::invalid_argument("Sphere radius must be finite and positive");
                }
            } else if constexpr (std::same_as<Shape, CapsuleShape3D>) {
                if (!std::isfinite(typedShape.halfHeight) || typedShape.halfHeight <= 0.0F ||
                    !std::isfinite(typedShape.radius) || typedShape.radius <= 0.0F) {
                    throw std::invalid_argument("Capsule dimensions must be finite and positive");
                }
            } else if constexpr (std::same_as<Shape, CylinderShape3D>) {
                if (!std::isfinite(typedShape.halfHeight)
                    || typedShape.halfHeight <= 0.0F
                    || !std::isfinite(typedShape.radius)
                    || typedShape.radius <= 0.0F) {
                    throw std::invalid_argument(
                        "Cylinder dimensions must be finite and positive"
                    );
                }
            } else if constexpr (std::same_as<Shape, MeshShape3D>) {
                if (typedShape.vertices.empty()
                    || typedShape.indices.empty()
                    || typedShape.indices.size() % 3 != 0) {
                    throw std::invalid_argument(
                        "Mesh colliders require indexed triangle geometry"
                    );
                }
                for (const math::Vec3& vertex : typedShape.vertices) {
                    if (!finite(vertex)) {
                        throw std::invalid_argument(
                            "Mesh collider vertices must be finite"
                        );
                    }
                }
                for (const std::uint32_t index : typedShape.indices) {
                    if (index >= typedShape.vertices.size()) {
                        throw std::invalid_argument(
                            "Mesh collider index is out of range"
                        );
                    }
                }
            } else if constexpr (std::same_as<Shape, ConvexShape3D>) {
                if (typedShape.points.size() < 4) {
                    throw std::invalid_argument(
                        "Convex colliders require at least four points"
                    );
                }
                for (const math::Vec3& point : typedShape.points) {
                    if (!finite(point)) {
                        throw std::invalid_argument(
                            "Convex collider points must be finite"
                        );
                    }
                }
            }
        },
        shape
    );
}

[[nodiscard]] bool collisionFiltersAllow(
    const JPH::CollisionGroup& first,
    const JPH::CollisionGroup& second
) noexcept {
    return (first.GetGroupID() & second.GetSubGroupID()) != 0 &&
        (second.GetGroupID() & first.GetSubGroupID()) != 0;
}

class QueryBodyFilter final : public JPH::BodyFilter {
public:
    explicit QueryBodyFilter(const CollisionFilter& filter) : m_filter(filter) {}

    [[nodiscard]] bool ShouldCollideLocked(const JPH::Body& body) const override {
        const JPH::CollisionGroup& group = body.GetCollisionGroup();
        return (m_filter.layer & group.GetSubGroupID()) != 0 &&
            (group.GetGroupID() & m_filter.mask) != 0;
    }

private:
    CollisionFilter m_filter;
};

} // namespace

struct PhysicsWorld3D::Impl {
    struct BodySlot {
        JPH::BodyID body;
        std::uint32_t generation = 1;
        BodyType type = BodyType::Static;
        bool active = false;
        bool added = false;
    };

    class ContactBridge final : public JPH::ContactListener {
    public:
        explicit ContactBridge(Impl& owner) : m_owner(owner) {}

        JPH::ValidateResult OnContactValidate(
            const JPH::Body& first,
            const JPH::Body& second,
            JPH::RVec3Arg,
            const JPH::CollideShapeResult&
        ) override {
            return collisionFiltersAllow(
                first.GetCollisionGroup(),
                second.GetCollisionGroup()
            ) ? JPH::ValidateResult::AcceptAllContactsForThisBodyPair
              : JPH::ValidateResult::RejectAllContactsForThisBodyPair;
        }

        void OnContactAdded(
            const JPH::Body& first,
            const JPH::Body& second,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings&
        ) override {
            push(first, second, manifold, ContactPhase::Began);
        }

        void OnContactPersisted(
            const JPH::Body& first,
            const JPH::Body& second,
            const JPH::ContactManifold& manifold,
            JPH::ContactSettings&
        ) override {
            push(first, second, manifold, ContactPhase::Persisted);
        }

        void OnContactRemoved(const JPH::SubShapeIDPair& pair) override {
            if (!m_owner.contactListener) {
                return;
            }
            const BodyId3D first = m_owner.handleForNative(pair.GetBody1ID());
            const BodyId3D second = m_owner.handleForNative(pair.GetBody2ID());
            if (first && second) {
                std::scoped_lock lock(m_owner.contactMutex);
                m_owner.pendingContacts.push_back({
                    .firstBody = first,
                    .secondBody = second,
                    .phase = ContactPhase::Ended,
                });
            }
        }

    private:
        void push(
            const JPH::Body& first,
            const JPH::Body& second,
            const JPH::ContactManifold& manifold,
            const ContactPhase phase
        ) {
            if (!m_owner.contactListener) {
                return;
            }
            const BodyId3D firstHandle = m_owner.handleForNative(first.GetID());
            const BodyId3D secondHandle = m_owner.handleForNative(second.GetID());
            if (!firstHandle || !secondHandle) {
                return;
            }
            ContactEvent3D event{
                .firstBody = firstHandle,
                .secondBody = secondHandle,
                .normal = fromJoltVector(manifold.mWorldSpaceNormal),
                .phase = phase,
            };
            if (!manifold.mRelativeContactPointsOn1.empty()) {
                event.point = fromJoltPosition(manifold.GetWorldSpaceContactPointOn1(0));
            }
            std::scoped_lock lock(m_owner.contactMutex);
            m_owner.pendingContacts.push_back(event);
        }

        Impl& m_owner;
    };

    explicit Impl(const PhysicsWorld3DSettings& settings)
        : worldToken(nextWorldToken()),
          tempAllocator(10 * 1024 * 1024),
          jobSystem(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1),
          contactBridge(*this) {
        if (!finite(settings.gravity) || settings.maxBodies == 0 ||
            settings.maxBodies > JPH::BodyID::cMaxBodyIndex) {
            throw std::invalid_argument("Physics world settings are invalid");
        }
        const JPH::uint maxBodies = settings.maxBodies;
        const JPH::uint maxBodyPairs = std::max<JPH::uint>(maxBodies, 1'024);
        const JPH::uint maxContacts = std::max<JPH::uint>(
            std::min<JPH::uint>(maxBodies, 10'240),
            1'024
        );
        physicsSystem.Init(
            maxBodies,
            0,
            maxBodyPairs,
            maxContacts,
            broadPhaseLayerInterface,
            objectVsBroadPhaseFilter,
            objectLayerPairFilter
        );
        physicsSystem.SetGravity(toJoltVector(settings.gravity));
        physicsSystem.SetContactListener(&contactBridge);
    }

    ~Impl() {
        JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();
        for (const BodySlot& slot : bodies) {
            if (!slot.active) {
                continue;
            }
            if (slot.added) {
                bodyInterface.RemoveBody(slot.body);
            }
            bodyInterface.DestroyBody(slot.body);
        }
    }

    [[nodiscard]] bool contains(const PhysicsBody3D handle) const noexcept {
        const BodyId3D id = handle.id();
        if (id.world != worldToken || id.index == 0 || id.index > bodies.size()) {
            return false;
        }
        const BodySlot& slot = bodies[id.index - 1];
        return slot.active && slot.generation == id.generation;
    }

    [[nodiscard]] BodySlot& require(const PhysicsBody3D handle) {
        if (!contains(handle)) {
            throw std::invalid_argument("Physics body handle is invalid or no longer alive");
        }
        return bodies[handle.id().index - 1];
    }

    [[nodiscard]] const BodySlot& require(const PhysicsBody3D handle) const {
        if (!contains(handle)) {
            throw std::invalid_argument("Physics body handle is invalid or no longer alive");
        }
        return bodies[handle.id().index - 1];
    }

    [[nodiscard]] BodyId3D handleForNative(const JPH::BodyID native) const noexcept {
        const auto found = nativeBodies.find(native.GetIndexAndSequenceNumber());
        return found == nativeBodies.end() ? BodyId3D{} : found->second;
    }

    void dispatchContacts() {
        std::vector<ContactEvent3D> contacts;
        {
            std::scoped_lock lock(contactMutex);
            contacts.swap(pendingContacts);
        }
        if (contactListener) {
            for (const ContactEvent3D& event : contacts) {
                contactListener(event);
            }
        }
    }

    std::uint32_t worldToken = 0;
    BroadPhaseLayerInterface broadPhaseLayerInterface;
    ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;
    ObjectLayerPairFilter objectLayerPairFilter;
    JPH::TempAllocatorImpl tempAllocator;
    JPH::JobSystemThreadPool jobSystem;
    JPH::PhysicsSystem physicsSystem;
    std::vector<BodySlot> bodies;
    std::vector<std::uint32_t> freeSlots;
    std::unordered_map<std::uint32_t, BodyId3D> nativeBodies;
    ContactListener3D contactListener;
    std::mutex contactMutex;
    std::vector<ContactEvent3D> pendingContacts;
    ContactBridge contactBridge;
};

PhysicsWorld3D::PhysicsWorld3D(const PhysicsWorld3DSettings& settings) {
    // Jolt's allocator, factory, and reflected types must exist before any
    // Jolt-backed member of Impl is constructed.
    ensureJoltRuntime();
    m_impl = std::make_unique<Impl>(settings);
}

PhysicsWorld3D::~PhysicsWorld3D() = default;
PhysicsWorld3D::PhysicsWorld3D(PhysicsWorld3D&&) noexcept = default;
PhysicsWorld3D& PhysicsWorld3D::operator=(PhysicsWorld3D&&) noexcept = default;

PhysicsBody3D PhysicsWorld3D::createBody(
    const PhysicsBody3DSettings& settings,
    const PhysicsShape3D& shape,
    const PhysicsMaterial3D& material,
    const math::Vec3& colliderOffset,
    const bool sensor
) {
    validateBodySettings(settings);
    validateShape(shape);
    validateMaterial(material);
    if (!finite(colliderOffset)) {
        throw std::invalid_argument("Collider offset must be finite");
    }

    JPH::ShapeRefC nativeShape;
    std::visit(
        [&](const auto& typedShape) {
            using Shape = std::remove_cvref_t<decltype(typedShape)>;
            if constexpr (std::same_as<Shape, BoxShape3D>) {
                const float smallestExtent = std::min({
                    typedShape.halfExtents.x,
                    typedShape.halfExtents.y,
                    typedShape.halfExtents.z
                });
                nativeShape = new JPH::BoxShape(
                    toJoltVector(typedShape.halfExtents),
                    std::min(JPH::cDefaultConvexRadius, smallestExtent * 0.5F)
                );
            } else if constexpr (std::same_as<Shape, SphereShape3D>) {
                nativeShape = new JPH::SphereShape(typedShape.radius);
            } else if constexpr (std::same_as<Shape, CapsuleShape3D>) {
                nativeShape = new JPH::CapsuleShape(
                    typedShape.halfHeight,
                    typedShape.radius
                );
            } else if constexpr (std::same_as<Shape, CylinderShape3D>) {
                nativeShape = new JPH::CylinderShape(
                    typedShape.halfHeight,
                    typedShape.radius,
                    std::min(
                        JPH::cDefaultConvexRadius,
                        std::min(typedShape.halfHeight, typedShape.radius) * 0.5F
                    )
                );
            } else if constexpr (std::same_as<Shape, MeshShape3D>) {
                JPH::VertexList vertices;
                vertices.reserve(typedShape.vertices.size());
                for (const math::Vec3& vertex : typedShape.vertices) {
                    vertices.emplace_back(vertex.x, vertex.y, vertex.z);
                }
                JPH::IndexedTriangleList triangles;
                triangles.reserve(typedShape.indices.size() / 3);
                for (std::size_t index = 0;
                     index < typedShape.indices.size(); index += 3) {
                    triangles.emplace_back(
                        typedShape.indices[index],
                        typedShape.indices[index + 1],
                        typedShape.indices[index + 2],
                        0
                    );
                }
                JPH::Shape::ShapeResult result = JPH::MeshShapeSettings(
                    std::move(vertices),
                    std::move(triangles)
                ).Create();
                if (!result.IsValid()) {
                    throw std::invalid_argument(
                        "Failed to create mesh collider: "
                        + std::string(result.GetError().c_str())
                    );
                }
                nativeShape = result.Get();
            } else if constexpr (std::same_as<Shape, ConvexShape3D>) {
                JPH::Array<JPH::Vec3> points;
                points.reserve(typedShape.points.size());
                for (const math::Vec3& point : typedShape.points) {
                    points.emplace_back(point.x, point.y, point.z);
                }
                JPH::Shape::ShapeResult result =
                    JPH::ConvexHullShapeSettings(points).Create();
                if (!result.IsValid()) {
                    throw std::invalid_argument(
                        "Failed to create convex collider: "
                        + std::string(result.GetError().c_str())
                    );
                }
                nativeShape = result.Get();
            }
        },
        shape
    );
    if (colliderOffset != math::Vec3{0.0F}) {
        nativeShape = new JPH::RotatedTranslatedShape(
            toJoltVector(colliderOffset),
            JPH::Quat::sIdentity(),
            nativeShape.GetPtr()
        );
    }

    JPH::BodyCreationSettings bodySettings(
        nativeShape.GetPtr(),
        toJoltPosition(settings.position),
        toJoltRotation(settings.rotation),
        toJoltMotionType(settings.type),
        toJoltObjectLayer(settings.type)
    );
    if (std::holds_alternative<MeshShape3D>(shape)
        && settings.type != BodyType::Static) {
        throw std::invalid_argument(
            "MeshCollider3DComponent requires a static rigid body; "
            "use ConvexCollider3DComponent for moving bodies"
        );
    }
    bodySettings.mLinearVelocity = toJoltVector(settings.linearVelocity);
    bodySettings.mAngularVelocity = toJoltVector(settings.angularVelocity);
    bodySettings.mUserData = settings.userData;
    bodySettings.mCollisionGroup = JPH::CollisionGroup(
        nullptr,
        settings.collision.layer,
        settings.collision.mask
    );
    bodySettings.mIsSensor = sensor;
    bodySettings.mCollideKinematicVsNonDynamic = settings.type == BodyType::Kinematic;
    bodySettings.mMotionQuality = settings.continuousCollision
        ? JPH::EMotionQuality::LinearCast
        : JPH::EMotionQuality::Discrete;
    bodySettings.mFriction = material.friction;
    bodySettings.mRestitution = material.restitution;
    bodySettings.mLinearDamping = settings.linearDamping;
    bodySettings.mAngularDamping = settings.angularDamping;
    bodySettings.mGravityFactor = settings.gravityScale;
    if (settings.type == BodyType::Dynamic) {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = settings.mass;
    }

    JPH::BodyInterface& bodyInterface = m_impl->physicsSystem.GetBodyInterface();
    JPH::Body* nativeBody = bodyInterface.CreateBody(bodySettings);
    if (nativeBody == nullptr) {
        throw std::runtime_error("Jolt failed to create a physics body");
    }
    const JPH::BodyID nativeId = nativeBody->GetID();
    if (settings.enabled) {
        bodyInterface.AddBody(
            nativeId,
            settings.type == BodyType::Static
                ? JPH::EActivation::DontActivate
                : JPH::EActivation::Activate
        );
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
    slot.body = nativeId;
    slot.type = settings.type;
    slot.active = true;
    slot.added = settings.enabled;
    const BodyId3D id{m_impl->worldToken, slotIndex + 1, slot.generation};
    m_impl->nativeBodies.emplace(nativeId.GetIndexAndSequenceNumber(), id);
    return PhysicsBody3D(id);
}

void PhysicsWorld3D::destroyBody(const PhysicsBody3D body) {
    Impl::BodySlot& slot = m_impl->require(body);
    JPH::BodyInterface& bodyInterface = m_impl->physicsSystem.GetBodyInterface();
    m_impl->nativeBodies.erase(slot.body.GetIndexAndSequenceNumber());
    if (slot.added) {
        bodyInterface.RemoveBody(slot.body);
    }
    bodyInterface.DestroyBody(slot.body);
    slot.body = JPH::BodyID();
    slot.active = false;
    slot.added = false;
    ++slot.generation;
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    m_impl->freeSlots.push_back(body.id().index - 1);
}

bool PhysicsWorld3D::contains(const PhysicsBody3D body) const noexcept {
    return m_impl != nullptr && m_impl->contains(body);
}

void PhysicsWorld3D::step(const float fixedDeltaTime) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0F) {
        throw std::invalid_argument("Physics time step must be finite and positive");
    }
    const JPH::EPhysicsUpdateError error = m_impl->physicsSystem.Update(
        fixedDeltaTime,
        1,
        &m_impl->tempAllocator,
        &m_impl->jobSystem
    );
    m_impl->dispatchContacts();
    if (error != JPH::EPhysicsUpdateError::None) {
        throw std::runtime_error("Jolt exhausted a simulation contact or body-pair buffer");
    }
}

void PhysicsWorld3D::setGravity(const math::Vec3& gravity) {
    if (!finite(gravity)) {
        throw std::invalid_argument("Physics world gravity must be finite");
    }
    m_impl->physicsSystem.SetGravity(toJoltVector(gravity));
}

math::Vec3 PhysicsWorld3D::gravity() const {
    return fromJoltVector(m_impl->physicsSystem.GetGravity());
}

void PhysicsWorld3D::setTransform(
    const PhysicsBody3D body,
    const math::Vec3& position,
    const math::Quat& rotation
) {
    if (!finite(position)) {
        throw std::invalid_argument("Physics position must be finite");
    }
    const Impl::BodySlot& slot = m_impl->require(body);
    m_impl->physicsSystem.GetBodyInterface().SetPositionAndRotationWhenChanged(
        slot.body,
        toJoltPosition(position),
        toJoltRotation(rotation),
        slot.type == BodyType::Static
            ? JPH::EActivation::DontActivate
            : JPH::EActivation::Activate
    );
}

math::Vec3 PhysicsWorld3D::position(const PhysicsBody3D body) const {
    JPH::RVec3 position;
    JPH::Quat nativeRotation;
    m_impl->physicsSystem.GetBodyInterface().GetPositionAndRotation(
        m_impl->require(body).body,
        position,
        nativeRotation
    );
    return fromJoltPosition(position);
}

math::Quat PhysicsWorld3D::rotation(const PhysicsBody3D body) const {
    JPH::RVec3 nativePosition;
    JPH::Quat rotation;
    m_impl->physicsSystem.GetBodyInterface().GetPositionAndRotation(
        m_impl->require(body).body,
        nativePosition,
        rotation
    );
    return fromJoltRotation(rotation);
}

void PhysicsWorld3D::setLinearVelocity(
    const PhysicsBody3D body,
    const math::Vec3& velocity
) {
    if (!finite(velocity)) {
        throw std::invalid_argument("Physics velocity must be finite");
    }
    const Impl::BodySlot& slot = m_impl->require(body);
    if (slot.type == BodyType::Static) {
        throw std::invalid_argument("A static physics body cannot have velocity");
    }
    m_impl->physicsSystem.GetBodyInterface().SetLinearVelocity(
        slot.body,
        toJoltVector(velocity)
    );
}

math::Vec3 PhysicsWorld3D::linearVelocity(const PhysicsBody3D body) const {
    const Impl::BodySlot& slot = m_impl->require(body);
    if (slot.type == BodyType::Static) {
        return math::Vec3{0.0F};
    }
    return fromJoltVector(
        m_impl->physicsSystem.GetBodyInterface().GetLinearVelocity(slot.body)
    );
}

void PhysicsWorld3D::applyForce(const PhysicsBody3D body, const math::Vec3& force) {
    if (!finite(force)) {
        throw std::invalid_argument("Physics force must be finite");
    }
    const Impl::BodySlot& slot = m_impl->require(body);
    if (slot.type != BodyType::Dynamic) {
        throw std::invalid_argument("Forces can only be applied to dynamic bodies");
    }
    m_impl->physicsSystem.GetBodyInterface().AddForce(
        slot.body,
        toJoltVector(force),
        JPH::EActivation::Activate
    );
}

void PhysicsWorld3D::applyImpulse(const PhysicsBody3D body, const math::Vec3& impulse) {
    if (!finite(impulse)) {
        throw std::invalid_argument("Physics impulse must be finite");
    }
    const Impl::BodySlot& slot = m_impl->require(body);
    if (slot.type != BodyType::Dynamic) {
        throw std::invalid_argument("Impulses can only be applied to dynamic bodies");
    }
    m_impl->physicsSystem.GetBodyInterface().AddImpulse(
        slot.body,
        toJoltVector(impulse)
    );
}

std::optional<RaycastHit3D> PhysicsWorld3D::raycast(const RaycastQuery3D& query) const {
    if (!finite(query.origin) || !finite(query.displacement)) {
        throw std::invalid_argument("Physics ray must contain finite values");
    }
    if (query.displacement == math::Vec3{0.0F}) {
        return std::nullopt;
    }
    const JPH::RRayCast ray(
        toJoltPosition(query.origin),
        toJoltVector(query.displacement)
    );
    JPH::RayCastResult result;
    const QueryBodyFilter filter(query.filter);
    if (!m_impl->physicsSystem.GetNarrowPhaseQuery().CastRay(
            ray,
            result,
            {},
            {},
            filter
        )) {
        return std::nullopt;
    }
    const BodyId3D body = m_impl->handleForNative(result.mBodyID);
    if (!body) {
        return std::nullopt;
    }
    const JPH::RVec3 point = ray.GetPointOnRay(result.mFraction);
    math::Vec3 normal{0.0F};
    JPH::BodyLockRead lock(
        m_impl->physicsSystem.GetBodyLockInterface(),
        result.mBodyID
    );
    if (lock.Succeeded()) {
        normal = fromJoltVector(
            lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point)
        );
    }
    return RaycastHit3D{
        .body = body,
        .point = fromJoltPosition(point),
        .normal = normal,
        .fraction = result.mFraction,
    };
}

void PhysicsWorld3D::setContactListener(ContactListener3D listener) {
    m_impl->contactListener = std::move(listener);
    if (!m_impl->contactListener) {
        std::scoped_lock lock(m_impl->contactMutex);
        m_impl->pendingContacts.clear();
    }
}

} // namespace vshade::physics
