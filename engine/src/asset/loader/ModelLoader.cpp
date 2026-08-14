#include "asset/loader/ModelLoader.hpp"

#include "asset/loader/GltfLoaderUtils.hpp"
#include "renderer/Material.hpp"
#include "renderer/Model.hpp"
#include "renderer/Texture.hpp"

#include <fastgltf/types.hpp>

#if defined(_MSC_VER)
    #pragma warning(push, 0)
#endif
#include <stb_image.h>
#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vshade::asset {
namespace {

using TextureCache = std::vector<std::shared_ptr<renderer::Texture2D>>;

[[nodiscard]] std::span<const std::byte> sourceBytes(
    const fastgltf::DataSource& source
) {
    return std::visit(
        fastgltf::visitor{
            [](const fastgltf::sources::Array& data) {
                return std::span<const std::byte>(data.bytes.data(), data.bytes.size());
            },
            [](const fastgltf::sources::Vector& data) {
                return std::span<const std::byte>(data.bytes.data(), data.bytes.size());
            },
            [](const fastgltf::sources::ByteView& data) {
                return std::span<const std::byte>(data.bytes.data(), data.bytes.size());
            },
            [](const auto&) -> std::span<const std::byte> {
                throw std::invalid_argument("Unsupported glTF image data source");
            },
        },
        source
    );
}

[[nodiscard]] std::span<const std::byte> imageBytes(
    const fastgltf::Asset& asset,
    const fastgltf::Image& image
) {
    if (const auto* source = std::get_if<fastgltf::sources::BufferView>(&image.data)) {
        if (source->bufferViewIndex >= asset.bufferViews.size()) {
            throw std::invalid_argument("A glTF image references a missing buffer view");
        }
        const fastgltf::BufferView& view = asset.bufferViews[source->bufferViewIndex];
        if (view.bufferIndex >= asset.buffers.size()) {
            throw std::invalid_argument("A glTF image references a missing buffer");
        }
        const std::span<const std::byte> buffer =
            sourceBytes(asset.buffers[view.bufferIndex].data);
        if (view.byteOffset > buffer.size() ||
            view.byteLength > buffer.size() - view.byteOffset) {
            throw std::invalid_argument("A glTF image buffer view exceeds its buffer");
        }
        return buffer.subspan(view.byteOffset, view.byteLength);
    }
    return sourceBytes(image.data);
}

[[nodiscard]] renderer::TextureFilter textureFilter(
    const fastgltf::Asset& asset,
    const fastgltf::Texture& texture
) {
    if (!texture.samplerIndex.has_value()) {
        return renderer::TextureFilter::Linear;
    }
    if (texture.samplerIndex.value() >= asset.samplers.size()) {
        throw std::invalid_argument("A glTF texture references a missing sampler");
    }
    const fastgltf::Sampler& sampler = asset.samplers[texture.samplerIndex.value()];
    return sampler.magFilter == fastgltf::Filter::Nearest
        ? renderer::TextureFilter::Nearest
        : renderer::TextureFilter::Linear;
}

[[nodiscard]] renderer::TextureWrap textureWrap(
    const fastgltf::Asset& asset,
    const fastgltf::Texture& texture
) {
    if (!texture.samplerIndex.has_value()) {
        return renderer::TextureWrap::Repeat;
    }
    const fastgltf::Sampler& sampler = asset.samplers.at(texture.samplerIndex.value());
    return sampler.wrapS == fastgltf::Wrap::ClampToEdge ||
           sampler.wrapT == fastgltf::Wrap::ClampToEdge
        ? renderer::TextureWrap::ClampToEdge
        : renderer::TextureWrap::Repeat;
}

[[nodiscard]] std::shared_ptr<renderer::Texture2D> loadTexture(
    const fastgltf::Asset& asset,
    const std::size_t textureIndex,
    TextureCache& cache
) {
    if (textureIndex >= asset.textures.size()) {
        throw std::invalid_argument("A glTF material references a missing texture");
    }
    if (cache[textureIndex]) {
        return cache[textureIndex];
    }

    const fastgltf::Texture& texture = asset.textures[textureIndex];
    if (!texture.imageIndex.has_value() || texture.imageIndex.value() >= asset.images.size()) {
        throw std::invalid_argument("A glTF texture references a missing image");
    }
    const std::span<const std::byte> encoded =
        imageBytes(asset, asset.images[texture.imageIndex.value()]);
    if (encoded.empty() ||
        encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("A glTF image has an invalid encoded size");
    }

    int width = 0;
    int height = 0;
    using PixelPointer = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
    PixelPointer pixels(
        stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(encoded.data()),
            static_cast<int>(encoded.size()),
            &width,
            &height,
            nullptr,
            STBI_rgb_alpha
        ),
        &stbi_image_free
    );
    if (!pixels) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            std::string("Failed to decode glTF image: ") +
            (reason != nullptr ? reason : "unknown image error")
        );
    }
    if (width <= 0 || height <= 0) {
        throw std::runtime_error("A decoded glTF image has invalid dimensions");
    }

    cache[textureIndex] = std::make_shared<renderer::Texture2D>(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        renderer::TextureFormat::RGBA8,
        pixels.get(),
        textureFilter(asset, texture),
        textureWrap(asset, texture)
    );
    return cache[textureIndex];
}

[[nodiscard]] std::shared_ptr<renderer::Material> importMaterial(
    const fastgltf::Asset& asset,
    const fastgltf::Material& source,
    TextureCache& textureCache
) {
    auto material = std::make_shared<renderer::Material>(
        source.unlit
            ? renderer::MaterialShading::Unlit
            : renderer::MaterialShading::Lit
    );
    const auto& color = source.pbrData.baseColorFactor;
    material->setAlbedoColor({
        static_cast<float>(color.x()),
        static_cast<float>(color.y()),
        static_cast<float>(color.z()),
        static_cast<float>(color.w()),
    });
    material->setRoughness(static_cast<float>(source.pbrData.roughnessFactor));
    material->setMetallic(static_cast<float>(source.pbrData.metallicFactor));
    if (source.pbrData.baseColorTexture.has_value()) {
        material->setAlbedoTexture(loadTexture(
            asset,
            source.pbrData.baseColorTexture->textureIndex,
            textureCache
        ));
    }
    return material;
}

[[nodiscard]] math::Transform importTransform(const fastgltf::Node& node) {
    const auto* transform = std::get_if<fastgltf::TRS>(&node.transform);
    if (transform == nullptr) {
        throw std::logic_error("fastgltf did not decompose a model node transform");
    }

    return math::Transform(
        {
            static_cast<float>(transform->translation.x()),
            static_cast<float>(transform->translation.y()),
            static_cast<float>(transform->translation.z()),
        },
        {
            static_cast<float>(transform->rotation.w()),
            static_cast<float>(transform->rotation.x()),
            static_cast<float>(transform->rotation.y()),
            static_cast<float>(transform->rotation.z()),
        },
        {
            static_cast<float>(transform->scale.x()),
            static_cast<float>(transform->scale.y()),
            static_cast<float>(transform->scale.z()),
        }
    );
}

[[nodiscard]] std::vector<std::size_t> modelRoots(const fastgltf::Asset& asset) {
    if (!asset.scenes.empty()) {
        const std::size_t sceneIndex = asset.defaultScene.value_or(0);
        if (sceneIndex >= asset.scenes.size()) {
            throw std::invalid_argument("A glTF asset references a missing default scene");
        }
        return {
            asset.scenes[sceneIndex].nodeIndices.begin(),
            asset.scenes[sceneIndex].nodeIndices.end(),
        };
    }

    std::vector<bool> isChild(asset.nodes.size(), false);
    for (const fastgltf::Node& node : asset.nodes) {
        for (const std::size_t child : node.children) {
            if (child >= asset.nodes.size()) {
                throw std::invalid_argument("A glTF node references a missing child");
            }
            isChild[child] = true;
        }
    }

    std::vector<std::size_t> roots;
    for (std::size_t node = 0; node < isChild.size(); ++node) {
        if (!isChild[node]) {
            roots.push_back(node);
        }
    }
    return roots;
}

} // namespace

std::shared_ptr<renderer::Model> ModelLoader::load(
    const std::filesystem::path& path
) const {
    const fastgltf::Asset asset = detail::loadGltfAsset(path);
    if (asset.meshes.empty()) {
        throw std::invalid_argument("A glTF model asset requires at least one mesh");
    }

    std::vector<std::shared_ptr<renderer::Material>> materials;
    materials.reserve(asset.materials.size());
    TextureCache textureCache(asset.textures.size());
    for (const fastgltf::Material& material : asset.materials) {
        materials.push_back(importMaterial(asset, material, textureCache));
    }
    const auto defaultMaterial =
        std::make_shared<renderer::Material>(renderer::MaterialShading::Lit);

    std::vector<renderer::ModelPrimitive> primitives;
    std::vector<std::vector<std::size_t>> meshPrimitiveIndices(asset.meshes.size());
    for (std::size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh& mesh = asset.meshes[meshIndex];
        auto& mappedPrimitives = meshPrimitiveIndices[meshIndex];
        mappedPrimitives.reserve(mesh.primitives.size());
        for (const fastgltf::Primitive& primitive : mesh.primitives) {
            std::shared_ptr<renderer::Material> material = defaultMaterial;
            if (primitive.materialIndex.has_value()) {
                if (primitive.materialIndex.value() >= materials.size()) {
                    throw std::invalid_argument(
                        "A glTF mesh primitive references a missing material"
                    );
                }
                material = materials[primitive.materialIndex.value()];
            }

            mappedPrimitives.push_back(primitives.size());
            primitives.push_back({
                .mesh = detail::loadGltfPrimitive(asset, primitive),
                .material = std::move(material),
            });
        }
    }

    std::vector<renderer::ModelNode> nodes;
    nodes.reserve(asset.nodes.size());
    for (const fastgltf::Node& node : asset.nodes) {
        std::vector<std::size_t> attachedPrimitives;
        if (node.meshIndex.has_value()) {
            if (node.meshIndex.value() >= meshPrimitiveIndices.size()) {
                throw std::invalid_argument("A glTF node references a missing mesh");
            }
            attachedPrimitives = meshPrimitiveIndices[node.meshIndex.value()];
        }

        nodes.push_back({
            .name = std::string(node.name),
            .localTransform = importTransform(node),
            .primitives = std::move(attachedPrimitives),
            .children = {node.children.begin(), node.children.end()},
        });
    }

    std::vector<std::size_t> roots = modelRoots(asset);
    if (nodes.empty()) {
        std::vector<std::size_t> allPrimitives(primitives.size());
        for (std::size_t index = 0; index < allPrimitives.size(); ++index) {
            allPrimitives[index] = index;
        }
        nodes.push_back({
            .name = path.stem().string(),
            .primitives = std::move(allPrimitives),
        });
        roots.push_back(0);
    }

    return std::make_shared<renderer::Model>(
        std::move(primitives),
        std::move(nodes),
        std::move(roots)
    );
}

} // namespace vshade::asset
