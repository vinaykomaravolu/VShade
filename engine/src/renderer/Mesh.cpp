#include "renderer/Mesh.hpp"

#include <stdexcept>
#include <utility>

namespace vshade::renderer {

Mesh::Mesh(std::shared_ptr<VertexArray> vertexArray, const PrimitiveTopology topology)
    : m_vertexArray(std::move(vertexArray)),
      m_topology(topology) {
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

} // namespace vshade::renderer
