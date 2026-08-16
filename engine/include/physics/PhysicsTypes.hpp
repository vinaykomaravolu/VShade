#pragma once

#include <cstdint>

namespace vshade::physics {

/** @brief Describes how a rigid body is moved by the physics simulation. */
enum class BodyType : std::uint8_t {
    Static,
    Dynamic,
    Kinematic
};

/** @brief Identifies the stage of a collision contact notification. */
enum class ContactPhase : std::uint8_t {
    Began,
    Persisted,
    Ended
};

/** @brief Engine-facing collision layer and mask, independent of either backend. */
struct CollisionFilter {
    std::uint32_t layer = 1;
    std::uint32_t mask = 0xFFFFFFFFU;

    bool operator==(const CollisionFilter&) const = default;
};

/** @brief Opaque identifier for a body in a Box2D world. */
struct BodyId2D {
    std::uint32_t world = 0;
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] explicit operator bool() const noexcept {
        return world != 0 && generation != 0;
    }
    bool operator==(const BodyId2D&) const = default;
};

/** @brief Opaque identifier for a body in a Jolt world. */
struct BodyId3D {
    std::uint32_t world = 0;
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] explicit operator bool() const noexcept {
        return world != 0 && generation != 0;
    }
    bool operator==(const BodyId3D&) const = default;
};

} // namespace vshade::physics
