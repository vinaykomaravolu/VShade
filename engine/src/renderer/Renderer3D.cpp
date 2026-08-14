#include "renderer/Renderer3D.hpp"

#include "renderer/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace vshade::renderer {
namespace {

struct MeshCommand {
    math::Mat4 model{1.0F};
    const Mesh* mesh = nullptr;
    Material material;
    DrawParameters parameters;
};

namespace uniform {
constexpr std::string_view albedoColor = "albedoColor";
constexpr std::string_view albedoTexture = "albedoTexture";
constexpr std::string_view hasAlbedoTexture = "hasAlbedoTexture";
constexpr std::string_view normalTexture = "normalTexture";
constexpr std::string_view hasNormalTexture = "hasNormalTexture";
constexpr std::string_view normalScale = "normalScale";
constexpr std::string_view alphaMode = "alphaMode";
constexpr std::string_view alphaCutoff = "alphaCutoff";
constexpr std::string_view litMaterial = "litMaterial";
constexpr std::string_view lightDirection = "lightDirection";
constexpr std::string_view lightColor = "lightColor";
constexpr std::string_view lightIntensity = "lightIntensity";
constexpr std::string_view ambientLightColor = "ambientLightColor";
constexpr std::string_view ambientLightIntensity = "ambientLightIntensity";
constexpr std::string_view directionalLightCount = "directionalLightCount";
constexpr std::string_view pointLightCount = "pointLightCount";
constexpr std::string_view roughness = "roughness";
constexpr std::string_view metallic = "metallic";
} // namespace uniform

struct Renderer3DResources;

[[nodiscard]] Lighting defaultLighting() {
    Lighting lighting;
    lighting.addDirectionalLight(DirectionalLight{});
    return lighting;
}

struct Renderer3DState {
    math::Mat4 view{1.0F};
    math::Mat4 projection{1.0F};
    Lighting lighting = defaultLighting();
    std::vector<MeshCommand> commands;
    Renderer3DStats currentStats{};
    Renderer3DStats completedStats{};
    std::optional<PipelineStateGuard> pipelineStateGuard;
    std::unique_ptr<Renderer3DResources> resources;
    std::uint64_t resourceInitializationCount = 0;
    bool sceneActive = false;
};

[[nodiscard]] Renderer3DState& state() {
    static Renderer3DState instance;
    return instance;
}

void requireActiveScene() {
    if (!state().sceneActive) {
        throw std::logic_error("Renderer3D requires an active scene");
    }
}

void discardCurrentScene() noexcept {
    Renderer3DState& rendererState = state();
    rendererState.commands.clear();
    rendererState.currentStats = {};
    rendererState.sceneActive = false;
}

void queueMesh(
    const math::Mat4& model,
    const Mesh& mesh,
    const Material& material,
    const DrawParameters& parameters
) {
    Renderer3DState& rendererState = state();
    rendererState.commands.push_back({
        .model = model,
        .mesh = &mesh,
        .material = material,
        .parameters = parameters,
    });
    ++rendererState.currentStats.meshCount;
}

void queueModelNode(
    const Model& model,
    const std::size_t nodeIndex,
    const math::Mat4& parentTransform,
    const DrawParameters& parameters
) {
    const ModelNode& node = model.nodes()[nodeIndex];
    const math::Mat4 nodeTransform = parentTransform * node.localTransform.matrix();
    for (const std::size_t primitiveIndex : node.primitives) {
        const ModelPrimitive& primitive = model.primitives()[primitiveIndex];
        queueMesh(nodeTransform, *primitive.mesh, *primitive.material, parameters);
    }
    for (const std::size_t child : node.children) {
        queueModelNode(model, child, nodeTransform, parameters);
    }
}

[[nodiscard]] Shader createDefaultShader() {
    return Shader(
        "vshade-renderer3d",
        R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aTangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out mat3 vTangentBasis;
out vec2 vTexCoord;
out vec3 vWorldPosition;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 normal = normalize(normalMatrix * aNormal);
    vec3 tangent = normalize(mat3(model) * aTangent.xyz);
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = cross(normal, tangent) * aTangent.w;
    vTangentBasis = mat3(tangent, bitangent, normal);
    vTexCoord = aTexCoord;
    vec4 worldPosition = model * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;
    gl_Position = projection * view * worldPosition;
}
)glsl",
        R"glsl(#version 330 core
in mat3 vTangentBasis;
in vec2 vTexCoord;
in vec3 vWorldPosition;

const int maximumDirectionalLights = 4;
const int maximumPointLights = 16;

struct DirectionalLightData {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLightData {
    vec3 position;
    vec3 color;
    float intensity;
    float range;
};

uniform sampler2D albedoTexture;
uniform int hasAlbedoTexture;
uniform sampler2D normalTexture;
uniform int hasNormalTexture;
uniform float normalScale;
uniform int alphaMode;
uniform float alphaCutoff;
uniform int litMaterial;
uniform vec4 albedoColor;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 ambientLightColor;
uniform float ambientLightIntensity;
uniform int directionalLightCount;
uniform DirectionalLightData directionalLights[maximumDirectionalLights];
uniform int pointLightCount;
uniform PointLightData pointLights[maximumPointLights];
uniform float roughness;
uniform float metallic;

out vec4 fragmentColor;

void main() {
    vec4 sampledAlbedo = hasAlbedoTexture != 0
        ? texture(albedoTexture, vTexCoord)
        : vec4(1.0);
    vec4 albedo = sampledAlbedo * albedoColor;

    if (alphaMode == 1 && albedo.a < alphaCutoff) {
        discard;
    }

    if (litMaterial == 0) {
        fragmentColor = albedo;
        return;
    }

    vec3 tangentNormal = hasNormalTexture != 0
        ? texture(normalTexture, vTexCoord).xyz * 2.0 - 1.0
        : vec3(0.0, 0.0, 1.0);
    tangentNormal.xy *= normalScale;
    vec3 normalDirection = normalize(vTangentBasis * normalize(tangentNormal));
    if (!gl_FrontFacing) {
        normalDirection = -normalDirection;
    }
    // This is intentionally a small, non-PBR material model. Rough surfaces
    // scatter more light while metallic surfaces reduce diffuse response.
    float materialResponse = mix(1.15, 0.85, roughness);
    materialResponse *= mix(1.0, 0.85, metallic);
    vec3 lighting = ambientLightColor * ambientLightIntensity;

    for (int index = 0; index < directionalLightCount; ++index) {
        vec3 directionToLight = normalize(-directionalLights[index].direction);
        float diffuse = max(dot(normalDirection, directionToLight), 0.0);
        lighting += directionalLights[index].color *
            directionalLights[index].intensity * diffuse * materialResponse;
    }

    for (int index = 0; index < pointLightCount; ++index) {
        vec3 offsetToLight = pointLights[index].position - vWorldPosition;
        float distanceToLight = length(offsetToLight);
        vec3 directionToLight = distanceToLight > 0.000001
            ? offsetToLight / distanceToLight
            : normalDirection;
        float diffuse = max(dot(normalDirection, directionToLight), 0.0);
        float attenuation = clamp(
            1.0 - distanceToLight / pointLights[index].range,
            0.0,
            1.0
        );
        attenuation *= attenuation;
        lighting += pointLights[index].color * pointLights[index].intensity *
            diffuse * attenuation * materialResponse;
    }
    fragmentColor = vec4(albedo.rgb * lighting, albedo.a);
}
)glsl"
    );
}

struct Renderer3DResources {
    Shader defaultShader = createDefaultShader();
    Texture2D whiteTexture;
    Texture2D neutralNormalTexture;

    Renderer3DResources()
        : whiteTexture(
              1,
              1,
              TextureFormat::RGBA8,
              whitePixel.data(),
              TextureFilter::Nearest,
              TextureWrap::ClampToEdge
          ),
          neutralNormalTexture(
              1,
              1,
              TextureFormat::RGBA8,
              neutralNormalPixel.data(),
              TextureFilter::Nearest,
              TextureWrap::ClampToEdge
          ) {}

private:
    static constexpr std::array<std::uint8_t, 4> whitePixel{255, 255, 255, 255};
    static constexpr std::array<std::uint8_t, 4> neutralNormalPixel{128, 128, 255, 255};
};

[[nodiscard]] Renderer3DResources& resources() {
    Renderer3DState& rendererState = state();
    if (!rendererState.resources) {
        rendererState.resources = std::make_unique<Renderer3DResources>();
        ++rendererState.resourceInitializationCount;
    }
    return *rendererState.resources;
}

void validateShaderInterface(Shader& shader) {
    if (!shader.hasUniform(Renderer3DShaderInterface::model) ||
        !shader.hasUniform(Renderer3DShaderInterface::view) ||
        !shader.hasUniform(Renderer3DShaderInterface::projection)) {
        throw std::invalid_argument(
            "Renderer3D shaders require active model, view, and projection mat4 uniforms"
        );
    }
}

void applyParameters(
    Shader& shader,
    const MaterialParameters& parameters,
    std::uint32_t& nextTextureSlot
) {
    for (const auto& [name, parameter] : parameters.values()) {
        if (!shader.hasUniform(name)) {
            continue;
        }

        std::visit(
            [&shader, &name, &nextTextureSlot](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, int>) {
                    shader.setInt(name, value);
                } else if constexpr (std::is_same_v<Value, float>) {
                    shader.setFloat(name, value);
                } else if constexpr (std::is_same_v<Value, math::Vec2>) {
                    shader.setVec2(name, value);
                } else if constexpr (std::is_same_v<Value, math::Vec3>) {
                    shader.setVec3(name, value);
                } else if constexpr (std::is_same_v<Value, math::Vec4>) {
                    shader.setVec4(name, value);
                } else if constexpr (std::is_same_v<Value, math::Mat4>) {
                    shader.setMat4(name, value);
                } else {
                    value->bind(nextTextureSlot);
                    shader.setInt(name, static_cast<int>(nextTextureSlot));
                    ++nextTextureSlot;
                }
            },
            parameter
        );
    }
}

[[nodiscard]] std::string indexedLightUniform(
    const std::string_view array,
    const std::size_t index,
    const std::string_view field
) {
    return std::string(array) + "[" + std::to_string(index) + "]." + std::string(field);
}

void applyLighting(Shader& shader, const Lighting& lighting) {
    const AmbientLight ambient = lighting.ambientLight().value_or(
        AmbientLight{.color = {0.0F, 0.0F, 0.0F}, .intensity = 0.0F}
    );
    shader.setVec3(uniform::ambientLightColor, ambient.color);
    shader.setFloat(uniform::ambientLightIntensity, ambient.intensity);

    const auto& directionalLights = lighting.directionalLights();
    shader.setInt(
        uniform::directionalLightCount,
        static_cast<int>(directionalLights.size())
    );
    for (std::size_t index = 0; index < directionalLights.size(); ++index) {
        const DirectionalLight& light = directionalLights[index];
        shader.setVec3(indexedLightUniform("directionalLights", index, "direction"), light.direction);
        shader.setVec3(indexedLightUniform("directionalLights", index, "color"), light.color);
        shader.setFloat(indexedLightUniform("directionalLights", index, "intensity"), light.intensity);
    }

    const auto& pointLights = lighting.pointLights();
    shader.setInt(uniform::pointLightCount, static_cast<int>(pointLights.size()));
    for (std::size_t index = 0; index < pointLights.size(); ++index) {
        const PointLight& light = pointLights[index];
        shader.setVec3(indexedLightUniform("pointLights", index, "position"), light.position);
        shader.setVec3(indexedLightUniform("pointLights", index, "color"), light.color);
        shader.setFloat(indexedLightUniform("pointLights", index, "intensity"), light.intensity);
        shader.setFloat(indexedLightUniform("pointLights", index, "range"), light.range);
    }

    // Preserve the original custom-shader uniforms using the first directional light.
    const DirectionalLight legacy = directionalLights.empty()
        ? DirectionalLight{.color = {0.0F, 0.0F, 0.0F}, .intensity = 0.0F}
        : directionalLights.front();
    shader.setVec3(uniform::lightDirection, legacy.direction);
    shader.setVec3(uniform::lightColor, legacy.color);
    shader.setFloat(uniform::lightIntensity, legacy.intensity);
}

} // namespace

void Renderer3D::beginScene(const Camera& camera) {
    if (!Renderer::isInitialized()) {
        throw std::logic_error("Renderer3D requires an initialized Renderer");
    }
    Renderer3DState& rendererState = state();
    if (rendererState.sceneActive) {
        throw std::logic_error("Renderer3D scene is already active");
    }

    rendererState.pipelineStateGuard.emplace(Renderer::pushPipelineState());
    Renderer::setBlending(false);
    Renderer::setDepthTesting(true);
    Renderer::setDepthFunction(DepthFunction::Less);
    Renderer::setDepthWrite(true);
    Renderer::setFaceCulling(true);
    Renderer::setCullFace(CullFace::Back);
    Renderer::setFrontFace(FrontFace::CounterClockwise);

    rendererState.view = camera.view();
    rendererState.projection = camera.projection();
    rendererState.commands.clear();
    rendererState.currentStats = {};
    rendererState.currentStats.resourceInitializations =
        rendererState.resourceInitializationCount;
    rendererState.sceneActive = true;
}

void Renderer3D::setDirectionalLight(const DirectionalLight& light) {
    Lighting lighting = state().lighting;
    lighting.clearDirectionalLights();
    lighting.addDirectionalLight(light);
    state().lighting = std::move(lighting);
}

void Renderer3D::setLighting(const Lighting& lighting) {
    state().lighting = lighting;
}

void Renderer3D::drawMesh(
    const math::Transform& transform,
    const Mesh& mesh,
    const Material& material,
    const DrawParameters& parameters
) {
    requireActiveScene();
    queueMesh(transform.matrix(), mesh, material, parameters);
}

void Renderer3D::drawModel(
    const math::Transform& transform,
    const Model& model,
    const DrawParameters& parameters
) {
    requireActiveScene();
    const math::Mat4 worldTransform = transform.matrix();
    for (const std::size_t root : model.rootNodes()) {
        queueModelNode(model, root, worldTransform, parameters);
    }
}

void Renderer3D::endScene() {
    requireActiveScene();
    Renderer3DState& rendererState = state();

    try {
        if (!rendererState.commands.empty()) {
            Renderer3DResources& rendererResources = resources();
            std::stable_sort(
                rendererState.commands.begin(),
                rendererState.commands.end(),
                [&rendererState](const MeshCommand& left, const MeshCommand& right) {
                    const bool leftTransparent =
                        left.material.alphaMode() == MaterialAlphaMode::Blend;
                    const bool rightTransparent =
                        right.material.alphaMode() == MaterialAlphaMode::Blend;
                    if (leftTransparent != rightTransparent) {
                        return !leftTransparent;
                    }
                    if (leftTransparent) {
                        const float leftViewZ =
                            (rendererState.view * left.model * math::Vec4{0.0F, 0.0F, 0.0F, 1.0F}).z;
                        const float rightViewZ =
                            (rendererState.view * right.model * math::Vec4{0.0F, 0.0F, 0.0F, 1.0F}).z;
                        if (leftViewZ != rightViewZ) {
                            return leftViewZ < rightViewZ;
                        }
                    }
                    const std::uint32_t leftShader = left.material.hasShader()
                        ? left.material.shader()->rendererId()
                        : 0;
                    const std::uint32_t rightShader = right.material.hasShader()
                        ? right.material.shader()->rendererId()
                        : 0;
                    return leftShader < rightShader;
                }
            );

            for (const MeshCommand& command : rendererState.commands) {
                const bool transparent =
                    command.material.alphaMode() == MaterialAlphaMode::Blend;
                Renderer::setBlending(transparent);
                if (transparent) {
                    Renderer::setBlendFunction(
                        BlendFactor::SourceAlpha,
                        BlendFactor::OneMinusSourceAlpha
                    );
                }
                Renderer::setDepthWrite(!transparent);
                Renderer::setFaceCulling(!command.material.doubleSided());

                Shader& shader = command.material.hasShader()
                    ? *command.material.shader()
                    : rendererResources.defaultShader;
                validateShaderInterface(shader);
                shader.bind();
                shader.setVec4(uniform::albedoColor, command.material.albedoColor());
                shader.setFloat(uniform::roughness, command.material.roughness());
                shader.setFloat(uniform::metallic, command.material.metallic());
                shader.setInt(
                    uniform::alphaMode,
                    static_cast<int>(command.material.alphaMode())
                );
                shader.setFloat(uniform::alphaCutoff, command.material.alphaCutoff());
                shader.setFloat(uniform::normalScale, command.material.normalScale());
                shader.setInt(
                    uniform::litMaterial,
                    command.material.shading() == MaterialShading::Lit ? 1 : 0
                );
                applyLighting(shader, rendererState.lighting);
                shader.setInt(uniform::albedoTexture, 0);
                shader.setInt(uniform::normalTexture, 1);
                shader.setInt(
                    uniform::hasAlbedoTexture,
                    command.material.hasAlbedoTexture() ? 1 : 0
                );
                shader.setInt(
                    uniform::hasNormalTexture,
                    command.material.hasNormalTexture() ? 1 : 0
                );

                const Texture2D& texture = command.material.hasAlbedoTexture()
                    ? *command.material.albedoTexture()
                    : rendererResources.whiteTexture;
                texture.bind(0);

                const Texture2D& normalTexture = command.material.hasNormalTexture()
                    ? *command.material.normalTexture()
                    : rendererResources.neutralNormalTexture;
                normalTexture.bind(1);

                std::uint32_t nextTextureSlot = 2;
                applyParameters(shader, command.material.parameters(), nextTextureSlot);
                applyParameters(shader, command.parameters, nextTextureSlot);

                // Renderer-owned transforms cannot be replaced by custom values.
                shader.setMat4(Renderer3DShaderInterface::model, command.model);
                shader.setMat4(Renderer3DShaderInterface::view, rendererState.view);
                shader.setMat4(Renderer3DShaderInterface::projection, rendererState.projection);
                Renderer::draw(*command.mesh);
                ++rendererState.currentStats.drawCalls;
            }
        }

        rendererState.completedStats = rendererState.currentStats;
        rendererState.completedStats.resourceInitializations =
            rendererState.resourceInitializationCount;
        rendererState.pipelineStateGuard.reset();
        discardCurrentScene();
    } catch (...) {
        rendererState.pipelineStateGuard.reset();
        discardCurrentScene();
        throw;
    }
}

const Renderer3DStats& Renderer3D::stats() noexcept {
    return state().completedStats;
}

void Renderer3D::shutdown() noexcept {
    Renderer3DState& rendererState = state();
    rendererState.pipelineStateGuard.reset();
    discardCurrentScene();
    rendererState.resources.reset();
    rendererState.completedStats = {};
    rendererState.resourceInitializationCount = 0;
    try {
        rendererState.lighting = defaultLighting();
    } catch (...) {
        rendererState.lighting.clear();
    }
}

} // namespace vshade::renderer
