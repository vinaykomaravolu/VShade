#include "renderer/Model.hpp"

#include "renderer/Material.hpp"
#include "renderer/Mesh.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace vshade::renderer {
namespace {

void visitNode(
    const std::size_t nodeIndex,
    const std::vector<ModelNode>& nodes,
    std::vector<std::uint8_t>& visitState
) {
    if (visitState[nodeIndex] == 1) {
        throw std::invalid_argument("A model node hierarchy cannot contain a cycle");
    }
    if (visitState[nodeIndex] == 2) {
        return;
    }

    visitState[nodeIndex] = 1;
    for (const std::size_t child : nodes[nodeIndex].children) {
        visitNode(child, nodes, visitState);
    }
    visitState[nodeIndex] = 2;
}

void collectCollisionGeometry(
    const std::size_t nodeIndex,
    const math::Mat4& parentTransform,
    const std::vector<ModelNode>& nodes,
    const std::vector<ModelPrimitive>& primitives,
    std::vector<math::Vec3>& vertices,
    std::vector<std::uint32_t>& indices,
    ModelBounds& bounds,
    bool& hasBounds
) {
    const ModelNode& node = nodes[nodeIndex];
    const math::Mat4 nodeTransform = parentTransform * node.localTransform.matrix();
    for (const std::size_t primitiveIndex : node.primitives) {
        const Mesh& mesh = *primitives[primitiveIndex].mesh;
        if (mesh.topology() != PrimitiveTopology::Triangles
            || mesh.collisionVertices().empty()) {
            continue;
        }
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        for (const MeshVertex& vertex : mesh.collisionVertices()) {
            const math::Vec3 position = math::Vec3{
                nodeTransform * math::Vec4{vertex.position, 1.0F}
            };
            vertices.push_back(position);
            if (!hasBounds) {
                bounds.minimum = position;
                bounds.maximum = position;
                hasBounds = true;
            } else {
                bounds.minimum = glm::min(bounds.minimum, position);
                bounds.maximum = glm::max(bounds.maximum, position);
            }
        }
        for (const std::uint32_t index : mesh.collisionIndices()) {
            indices.push_back(base + index);
        }
    }
    for (const std::size_t child : node.children) {
        collectCollisionGeometry(
            child,
            nodeTransform,
            nodes,
            primitives,
            vertices,
            indices,
            bounds,
            hasBounds
        );
    }
}

} // namespace

Model::Model(
    std::vector<ModelPrimitive> primitives,
    std::vector<ModelNode> nodes,
    std::vector<std::size_t> rootNodes
)
    : m_primitives(std::move(primitives)),
      m_nodes(std::move(nodes)),
      m_rootNodes(std::move(rootNodes)) {
    for (const ModelPrimitive& primitive : m_primitives) {
        if (!primitive.mesh || !primitive.material) {
            throw std::invalid_argument(
                "A model primitive requires both mesh and material resources"
            );
        }
    }

    for (const ModelNode& node : m_nodes) {
        for (const std::size_t primitive : node.primitives) {
            if (primitive >= m_primitives.size()) {
                throw std::invalid_argument("A model node references a missing primitive");
            }
        }
        for (const std::size_t child : node.children) {
            if (child >= m_nodes.size()) {
                throw std::invalid_argument("A model node references a missing child");
            }
        }
    }

    for (const std::size_t root : m_rootNodes) {
        if (root >= m_nodes.size()) {
            throw std::invalid_argument("A model references a missing root node");
        }
    }

    std::vector<std::uint8_t> visitState(m_nodes.size(), 0);
    for (std::size_t node = 0; node < m_nodes.size(); ++node) {
        visitNode(node, m_nodes, visitState);
    }

    ModelBounds bounds;
    bool hasBounds = false;
    for (const std::size_t root : m_rootNodes) {
        collectCollisionGeometry(
            root,
            math::Mat4{1.0F},
            m_nodes,
            m_primitives,
            m_collisionVertices,
            m_collisionIndices,
            bounds,
            hasBounds
        );
    }
    if (hasBounds) {
        m_localBounds = bounds;
    }
}

const std::vector<ModelPrimitive>& Model::primitives() const noexcept {
    return m_primitives;
}

const std::vector<ModelNode>& Model::nodes() const noexcept {
    return m_nodes;
}

const std::vector<std::size_t>& Model::rootNodes() const noexcept {
    return m_rootNodes;
}

const std::optional<ModelBounds>& Model::localBounds() const noexcept {
    return m_localBounds;
}

const std::vector<math::Vec3>& Model::collisionVertices() const noexcept {
    return m_collisionVertices;
}

const std::vector<std::uint32_t>& Model::collisionIndices() const noexcept {
    return m_collisionIndices;
}

} // namespace vshade::renderer
