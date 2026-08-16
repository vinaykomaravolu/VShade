#version 330 core

in vec2 uv;
in vec3 worldNormal;
in vec3 worldPosition;

uniform sampler2D albedoTexture;
uniform int hasAlbedoTexture;
uniform vec4 albedoColor;
uniform vec3 ambientLightColor;
uniform float ambientLightIntensity;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 accentColor;
uniform float time;

out vec4 fragmentColor;

void main() {
    vec4 sampled = hasAlbedoTexture != 0
        ? texture(albedoTexture, uv)
        : vec4(1.0);
    vec4 albedo = sampled * albedoColor;

    vec3 normalDirection = normalize(worldNormal);
    float diffuse = max(dot(normalDirection, normalize(-lightDirection)), 0.0);
    vec3 lighting = ambientLightColor * ambientLightIntensity +
        lightColor * lightIntensity * diffuse;

    float pulse = 0.5 + 0.5 * sin(time * 3.0 + worldPosition.y * 2.0);
    float edge = pow(1.0 - abs(normalDirection.z), 3.0);
    vec3 customGlow = accentColor * edge * mix(0.15, 0.55, pulse);
    fragmentColor = vec4(albedo.rgb * lighting + customGlow, albedo.a);
}
