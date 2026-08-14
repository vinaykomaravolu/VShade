#include "renderer/model.hpp"

#include "renderer/material.hpp"
#include "renderer/mesh.hpp"

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

} // namespace vshade::renderer
