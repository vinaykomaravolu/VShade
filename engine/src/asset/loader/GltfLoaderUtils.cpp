#include "asset/loader/GltfLoaderUtils.hpp"

#include "renderer/Buffer.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/VertexArray.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vshade::asset::detail {
namespace {

[[nodiscard]] renderer::PrimitiveTopology topology(const fastgltf::PrimitiveType type) {
    if (type == fastgltf::PrimitiveType::Triangles) {
        return renderer::PrimitiveTopology::Triangles;
    }
    if (type == fastgltf::PrimitiveType::Lines) {
        return renderer::PrimitiveTopology::Lines;
    }
    if (type == fastgltf::PrimitiveType::Points) {
        return renderer::PrimitiveTopology::Points;
    }
    throw std::invalid_argument(
        "VShade does not yet support glTF strip, fan, or loop primitives"
    );
}

[[nodiscard]] std::runtime_error loadError(
    const std::filesystem::path& path,
    const fastgltf::Error error
) {
    return std::runtime_error(
        "Failed to load glTF asset '" + path.string() + "': " +
        std::string(fastgltf::getErrorMessage(error))
    );
}

} // namespace

fastgltf::Asset loadGltfAsset(const std::filesystem::path& path) {
    auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
    if (!gltfFile) {
        throw loadError(path, gltfFile.error());
    }

    constexpr auto extensions = fastgltf::Extensions::KHR_materials_unlit |
                                fastgltf::Extensions::KHR_mesh_quantization;
    fastgltf::Parser parser(extensions);
    constexpr auto options = fastgltf::Options::LoadExternalBuffers |
                             fastgltf::Options::LoadExternalImages |
                             fastgltf::Options::GenerateMeshIndices |
                             fastgltf::Options::DecomposeNodeMatrices;
    auto parsedAsset = parser.loadGltf(gltfFile.get(), path.parent_path(), options);
    if (parsedAsset.error() != fastgltf::Error::None) {
        throw loadError(path, parsedAsset.error());
    }
    return std::move(parsedAsset.get());
}

std::shared_ptr<renderer::Mesh> loadGltfPrimitive(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive
) {
    const auto* positionAttribute = primitive.findAttribute("POSITION");
    if (positionAttribute == primitive.attributes.end()) {
        throw std::invalid_argument("A glTF mesh primitive requires a POSITION attribute");
    }

    const fastgltf::Accessor& positionAccessor =
        asset.accessors.at(positionAttribute->accessorIndex);
    std::vector<renderer::MeshVertex> vertices(positionAccessor.count);
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset,
        positionAccessor,
        [&vertices](const fastgltf::math::fvec3 position, const std::size_t index) {
            vertices[index].position = {position.x(), position.y(), position.z()};
        }
    );

    if (const auto* normalAttribute = primitive.findAttribute("NORMAL");
        normalAttribute != primitive.attributes.end()) {
        const fastgltf::Accessor& normalAccessor =
            asset.accessors.at(normalAttribute->accessorIndex);
        if (normalAccessor.count != vertices.size()) {
            throw std::invalid_argument("glTF NORMAL and POSITION counts must match");
        }
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            asset,
            normalAccessor,
            [&vertices](const fastgltf::math::fvec3 normal, const std::size_t index) {
                vertices[index].normal = {normal.x(), normal.y(), normal.z()};
            }
        );
    }

    if (const auto* textureCoordinateAttribute = primitive.findAttribute("TEXCOORD_0");
        textureCoordinateAttribute != primitive.attributes.end()) {
        const fastgltf::Accessor& textureCoordinateAccessor =
            asset.accessors.at(textureCoordinateAttribute->accessorIndex);
        if (textureCoordinateAccessor.count != vertices.size()) {
            throw std::invalid_argument("glTF TEXCOORD_0 and POSITION counts must match");
        }
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
            asset,
            textureCoordinateAccessor,
            [&vertices](const fastgltf::math::fvec2 coordinate, const std::size_t index) {
                vertices[index].textureCoordinate = {coordinate.x(), coordinate.y()};
            }
        );
    }

    if (const auto* tangentAttribute = primitive.findAttribute("TANGENT");
        tangentAttribute != primitive.attributes.end()) {
        const fastgltf::Accessor& tangentAccessor =
            asset.accessors.at(tangentAttribute->accessorIndex);
        if (tangentAccessor.count != vertices.size()) {
            throw std::invalid_argument("glTF TANGENT and POSITION counts must match");
        }
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
            asset,
            tangentAccessor,
            [&vertices](const fastgltf::math::fvec4 tangent, const std::size_t index) {
                vertices[index].tangent = {
                    tangent.x(), tangent.y(), tangent.z(), tangent.w()
                };
            }
        );
    }

    if (!primitive.indicesAccessor.has_value()) {
        throw std::invalid_argument("Failed to generate indices for glTF mesh primitive");
    }
    const fastgltf::Accessor& indexAccessor =
        asset.accessors.at(primitive.indicesAccessor.value());
    std::vector<std::uint32_t> indices(indexAccessor.count);
    fastgltf::copyFromAccessor<std::uint32_t>(asset, indexAccessor, indices.data());

    for (const std::uint32_t index : indices) {
        if (index >= vertices.size()) {
            throw std::out_of_range("A glTF mesh index references a missing vertex");
        }
    }

    auto vertexBuffer = std::make_shared<renderer::VertexBuffer>(
        vertices.data(),
        vertices.size() * sizeof(renderer::MeshVertex)
    );
    vertexBuffer->setLayout({
        {"position", renderer::ShaderDataType::Float3},
        {"normal", renderer::ShaderDataType::Float3},
        {"textureCoordinate", renderer::ShaderDataType::Float2},
        {"tangent", renderer::ShaderDataType::Float4},
    });
    auto indexBuffer = std::make_shared<renderer::IndexBuffer>(
        indices.data(),
        indices.size()
    );
    auto vertexArray = std::make_shared<renderer::VertexArray>();
    vertexArray->addVertexBuffer(std::move(vertexBuffer));
    vertexArray->setIndexBuffer(std::move(indexBuffer));
    return std::make_shared<renderer::Mesh>(
        std::move(vertexArray),
        topology(primitive.type)
    );
}

} // namespace vshade::asset::detail
