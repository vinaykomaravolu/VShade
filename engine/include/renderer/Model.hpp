#pragma once

#include "math/Transform.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vshade::renderer {

class Material;
class Mesh;

/** @brief One independently drawable part of a model. */
struct ModelPrimitive {
    /** @brief Geometry used by this part. */
    std::shared_ptr<Mesh> mesh;
    /** @brief Surface properties used when drawing the geometry. */
    std::shared_ptr<Material> material;
};

/** @brief One node in a model-local transform hierarchy. */
struct ModelNode {
    /** @brief Optional source name useful for tools and animation lookup. */
    std::string name;
    /** @brief Transformation relative to this node's parent. */
    math::Transform localTransform;
    /** @brief Indices into Model::primitives() attached to this node. */
    std::vector<std::size_t> primitives;
    /** @brief Indices into Model::nodes() parented below this node. */
    std::vector<std::size_t> children;
};

/** Model-local axis-aligned bounds calculated from imported geometry. */
struct ModelBounds {
    math::Vec3 minimum{0.0F};
    math::Vec3 maximum{0.0F};
};

/**
 * @brief Renderable collection of mesh primitives, materials, and model nodes.
 *
 * A Model is an asset resource rather than a gameplay scene. Its nodes retain
 * the imported transform hierarchy but do not create scene entities.
 */
class Model final {
public:
    /**
     * @brief Creates and validates a complete model resource.
     * @throws std::invalid_argument If resources are null, indices are invalid,
     * or the node hierarchy contains a cycle.
     */
    Model(
        std::vector<ModelPrimitive> primitives,
        std::vector<ModelNode> nodes,
        std::vector<std::size_t> rootNodes
    );

    /** @brief Returns all independently drawable model parts. */
    [[nodiscard]] const std::vector<ModelPrimitive>& primitives() const noexcept;

    /** @brief Returns the model-local node hierarchy. */
    [[nodiscard]] const std::vector<ModelNode>& nodes() const noexcept;

    /** @brief Returns roots of the model's selected/default scene. */
    [[nodiscard]] const std::vector<std::size_t>& rootNodes() const noexcept;

    [[nodiscard]] const std::optional<ModelBounds>& localBounds() const noexcept;
    [[nodiscard]] const std::vector<math::Vec3>& collisionVertices() const noexcept;
    [[nodiscard]] const std::vector<std::uint32_t>& collisionIndices() const noexcept;

private:
    std::vector<ModelPrimitive> m_primitives;
    std::vector<ModelNode> m_nodes;
    std::vector<std::size_t> m_rootNodes;
    std::optional<ModelBounds> m_localBounds;
    std::vector<math::Vec3> m_collisionVertices;
    std::vector<std::uint32_t> m_collisionIndices;
};

} // namespace vshade::renderer
