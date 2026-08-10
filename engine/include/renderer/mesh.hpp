#pragma once

#include "renderer/rendertypes.hpp"
#include "renderer/vertexarray.hpp"

#include <memory>

namespace vshade::renderer {

/** @brief Renderable geometry backed by a vertex array. */
class Mesh final {
public:
    /**
     * @brief Creates a mesh and retains its vertex array resources.
     * @param vertexArray Vertex array containing the mesh geometry.
     * @param topology Primitive assembly mode used when drawing the mesh.
     * @throws std::invalid_argument If @p vertexArray is null.
     */
    explicit Mesh(
        std::shared_ptr<VertexArray> vertexArray,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    );

    /**
     * @brief Returns the retained vertex array.
     * @return Read-only reference to the shared vertex array.
     */
    [[nodiscard]] const std::shared_ptr<VertexArray>& vertexArray() const noexcept;

    /**
     * @brief Returns this mesh's primitive assembly mode.
     * @return Topology used when drawing the mesh.
     */
    [[nodiscard]] PrimitiveTopology topology() const noexcept;

private:
    std::shared_ptr<VertexArray> m_vertexArray;
    PrimitiveTopology m_topology = PrimitiveTopology::Triangles;
};

} // namespace vshade::renderer
