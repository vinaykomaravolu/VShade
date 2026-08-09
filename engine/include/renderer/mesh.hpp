#pragma once

#include "renderer/rendertypes.hpp"
#include "renderer/vertexarray.hpp"

#include <memory>

namespace vshade::renderer {

/** @brief Renderable geometry backed by a vertex array. */
class Mesh final {
public:
    /** @brief Creates a mesh and retains its vertex array resources. */
    explicit Mesh(
        std::shared_ptr<VertexArray> vertexArray,
        PrimitiveTopology topology = PrimitiveTopology::Triangles
    );

    [[nodiscard]] const std::shared_ptr<VertexArray>& vertexArray() const noexcept;
    [[nodiscard]] PrimitiveTopology topology() const noexcept;

private:
    std::shared_ptr<VertexArray> m_vertexArray;
    PrimitiveTopology m_topology = PrimitiveTopology::Triangles;
};

} // namespace vshade::renderer
