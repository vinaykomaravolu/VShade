#include "renderer/renderer3d.hpp"

#include "renderer/renderer.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace vshade::renderer {
namespace {

struct MeshCommand {
    math::Mat4 model{1.0F};
    const Mesh* mesh = nullptr;
    Material material;
};

struct Renderer3DState {
    math::Mat4 view{1.0F};
    math::Mat4 projection{1.0F};
    DirectionalLight light{};
    std::vector<MeshCommand> commands;
    Renderer3DStats currentStats{};
    Renderer3DStats completedStats{};
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
    vec3 lighting = vec3(0.15) + lightColor * lightIntensity * diffuse;
    fragmentColor = vec4(albedo.rgb * lighting, albedo.a);
}
)glsl"
    );
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
    rendererState.sceneActive = true;
}

void Renderer3D::setDirectionalLight(const DirectionalLight& light) {
    if (math::lengthSquared(light.direction) <= 0.000001F) {
        throw std::invalid_argument("Directional light direction must not be zero");
    }
    validateNonNegativeFinite(light.intensity, "Directional light intensity must be finite and non-negative");

    Renderer3DState& rendererState = state();
    rendererState.light = light;
    rendererState.light.direction = math::normalize(light.direction);
}

void Renderer3D::drawMesh(
    const math::Transform& transform,
    const Mesh& mesh,
    const Material& material
) {
    requireActiveScene();
    Renderer3DState& rendererState = state();
    rendererState.commands.push_back({
        .model = transform.matrix(),
        .mesh = &mesh,
        .material = material,
    });
    ++rendererState.currentStats.meshCount;
}

void Renderer3D::endScene() {
    requireActiveScene();
    Renderer3DState& rendererState = state();

    try {
        if (!rendererState.commands.empty()) {
            Shader defaultShader = createDefaultShader();
            constexpr std::array<std::uint8_t, 4> whitePixel{255, 255, 255, 255};
            Texture2D whiteTexture(
                1,
                1,
                TextureFormat::RGBA8,
                whitePixel.data(),
                TextureFilter::Nearest,
                TextureWrap::ClampToEdge
            );

            for (const MeshCommand& command : rendererState.commands) {
                Shader& shader = command.material.hasShader()
                    ? *command.material.shader()
                    : defaultShader;
                shader.setMat4("model", command.model);
                shader.setMat4("view", rendererState.view);
                shader.setMat4("projection", rendererState.projection);
                shader.setVec4("albedoColor", command.material.albedoColor());
                shader.setFloat("roughness", command.material.roughness());
                shader.setFloat("metallic", command.material.metallic());
                shader.setInt(
                    "litMaterial",
                    command.material.shading() == MaterialShading::Lit ? 1 : 0
                );
                shader.setVec3("lightDirection", rendererState.light.direction);
                shader.setVec3("lightColor", rendererState.light.color);
                shader.setFloat("lightIntensity", rendererState.light.intensity);
                shader.setInt("albedoTexture", 0);
                shader.setInt(
                    "hasAlbedoTexture",
                    command.material.hasAlbedoTexture() ? 1 : 0
                );

                const Texture2D& texture = command.material.hasAlbedoTexture()
                    ? *command.material.albedoTexture()
                    : whiteTexture;
                texture.bind(0);
                Renderer::draw(*command.mesh);
                ++rendererState.currentStats.drawCalls;
            }
        }

        rendererState.completedStats = rendererState.currentStats;
        discardCurrentScene();
    } catch (...) {
        discardCurrentScene();
        throw;
    }
}

const Renderer3DStats& Renderer3D::stats() noexcept {
    return state().completedStats;
}

} // namespace vshade::renderer
