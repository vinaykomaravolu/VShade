#include "renderer/renderer3d.hpp"

#include "renderer/renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
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
constexpr std::string_view litMaterial = "litMaterial";
constexpr std::string_view lightDirection = "lightDirection";
constexpr std::string_view lightColor = "lightColor";
constexpr std::string_view lightIntensity = "lightIntensity";
constexpr std::string_view roughness = "roughness";
constexpr std::string_view metallic = "metallic";
} // namespace uniform

struct Renderer3DResources;

struct Renderer3DState {
    math::Mat4 view{1.0F};
    math::Mat4 projection{1.0F};
    DirectionalLight light{};
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

void validateNonNegativeFinite(const float value, const char* message) {
    if (!std::isfinite(value) || value < 0.0F) {
        throw std::invalid_argument(message);
    }
}

[[nodiscard]] Shader createDefaultShader() {
    return Shader(
        "vshade-renderer3d",
        R"glsl(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vNormal = normalize(normalMatrix * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl",
        R"glsl(#version 330 core
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D albedoTexture;
uniform int hasAlbedoTexture;
uniform int litMaterial;
uniform vec4 albedoColor;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float roughness;
uniform float metallic;

out vec4 fragmentColor;

void main() {
    vec4 sampledAlbedo = hasAlbedoTexture != 0
        ? texture(albedoTexture, vTexCoord)
        : vec4(1.0);
    vec4 albedo = sampledAlbedo * albedoColor;

    if (litMaterial == 0) {
        fragmentColor = albedo;
        return;
    }

    vec3 normalDirection = normalize(vNormal);
    vec3 directionToLight = normalize(-lightDirection);
    float diffuse = max(dot(normalDirection, directionToLight), 0.0);
    // This is intentionally a small, non-PBR material model. Rough surfaces
    // scatter more light while metallic surfaces reduce diffuse response.
    diffuse *= mix(1.15, 0.85, roughness);
    diffuse *= mix(1.0, 0.85, metallic);
    vec3 lighting = vec3(0.15) + lightColor * lightIntensity * diffuse;
    fragmentColor = vec4(albedo.rgb * lighting, albedo.a);
}
)glsl"
    );
}

struct Renderer3DResources {
    Shader defaultShader = createDefaultShader();
    Texture2D whiteTexture;

    Renderer3DResources()
        : whiteTexture(
              1,
              1,
              TextureFormat::RGBA8,
              whitePixel.data(),
              TextureFilter::Nearest,
              TextureWrap::ClampToEdge
          ) {}

private:
    static constexpr std::array<std::uint8_t, 4> whitePixel{255, 255, 255, 255};
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
    if (!std::isfinite(light.direction.x) || !std::isfinite(light.direction.y) ||
        !std::isfinite(light.direction.z) ||
        math::lengthSquared(light.direction) <= 0.000001F) {
        throw std::invalid_argument("Directional light direction must not be zero");
    }
    validateNonNegativeFinite(light.color.r, "Directional light red must be finite and non-negative");
    validateNonNegativeFinite(light.color.g, "Directional light green must be finite and non-negative");
    validateNonNegativeFinite(light.color.b, "Directional light blue must be finite and non-negative");
    validateNonNegativeFinite(light.intensity, "Directional light intensity must be finite and non-negative");

    Renderer3DState& rendererState = state();
    rendererState.light = light;
    rendererState.light.direction = math::normalize(light.direction);
}

void Renderer3D::drawMesh(
    const math::Transform& transform,
    const Mesh& mesh,
    const Material& material,
    const DrawParameters& parameters
) {
    requireActiveScene();
    Renderer3DState& rendererState = state();
    rendererState.commands.push_back({
        .model = transform.matrix(),
        .mesh = &mesh,
        .material = material,
        .parameters = parameters,
    });
    ++rendererState.currentStats.meshCount;
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
                [](const MeshCommand& left, const MeshCommand& right) {
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
                Shader& shader = command.material.hasShader()
                    ? *command.material.shader()
                    : rendererResources.defaultShader;
                validateShaderInterface(shader);
                shader.bind();
                shader.setVec4(uniform::albedoColor, command.material.albedoColor());
                shader.setFloat(uniform::roughness, command.material.roughness());
                shader.setFloat(uniform::metallic, command.material.metallic());
                shader.setInt(
                    uniform::litMaterial,
                    command.material.shading() == MaterialShading::Lit ? 1 : 0
                );
                shader.setVec3(uniform::lightDirection, rendererState.light.direction);
                shader.setVec3(uniform::lightColor, rendererState.light.color);
                shader.setFloat(uniform::lightIntensity, rendererState.light.intensity);
                shader.setInt(uniform::albedoTexture, 0);
                shader.setInt(
                    uniform::hasAlbedoTexture,
                    command.material.hasAlbedoTexture() ? 1 : 0
                );

                const Texture2D& texture = command.material.hasAlbedoTexture()
                    ? *command.material.albedoTexture()
                    : rendererResources.whiteTexture;
                texture.bind(0);

                std::uint32_t nextTextureSlot = 1;
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
}

} // namespace vshade::renderer
