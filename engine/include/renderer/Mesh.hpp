#pragma once

#include "math/Vector.hpp"
#include "renderer/RenderTypes.hpp"
#include "renderer/VertexArray.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace vshade::renderer {

/** @brief Initial vertex format for basic 3D meshes. */
struct MeshVertex {
    /** @brief Vertex position in mesh-local space. */
    math::Vec3 position{0.0F};
    /** @brief Normalized surface direction in mesh-local space. */
    math::Vec3 normal{0.0F, 1.0F, 0.0F};
    /** @brief Two-dimensional texture coordinate. */
    math::Vec2 textureCoordinate{0.0F};
    /** @brief Tangent direction and handedness used by tangent-space normal maps. */
    math::Vec4 tangent{1.0F, 0.0F, 0.0F, 1.0F};
};

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
        PrimitiveTopology topology = PrimitiveTopology::Triangles,
        std::vector<MeshVertex> collisionVertices = {},
        std::vector<std::uint32_t> collisionIndices = {}
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

    /** CPU-side geometry retained for model bounds and editor physics shapes. */
    [[nodiscard]] const std::vector<MeshVertex>& collisionVertices() const noexcept;
    [[nodiscard]] const std::vector<std::uint32_t>& collisionIndices() const noexcept;

private:
    std::shared_ptr<VertexArray> m_vertexArray;
    PrimitiveTopology m_topology = PrimitiveTopology::Triangles;
    std::vector<MeshVertex> m_collisionVertices;
    std::vector<std::uint32_t> m_collisionIndices;
};

} // namespace vshade::renderer
