#include "renderer/Mesh.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::renderer {

Mesh::Mesh(
    std::shared_ptr<VertexArray> vertexArray,
    const PrimitiveTopology topology,
    std::vector<MeshVertex> collisionVertices,
    std::vector<std::uint32_t> collisionIndices
)
    : m_vertexArray(std::move(vertexArray)),
      m_topology(topology),
      m_collisionVertices(std::move(collisionVertices)),
      m_collisionIndices(std::move(collisionIndices)) {
    if (!m_vertexArray) {
        throw std::invalid_argument("Mesh vertex array must not be null");
    }
}

const std::shared_ptr<VertexArray>& Mesh::vertexArray() const noexcept {
    return m_vertexArray;
}

PrimitiveTopology Mesh::topology() const noexcept {
    return m_topology;
}

const std::vector<MeshVertex>& Mesh::collisionVertices() const noexcept {
    return m_collisionVertices;
}

const std::vector<std::uint32_t>& Mesh::collisionIndices() const noexcept {
    return m_collisionIndices;
}

} // namespace vshade::renderer
