#version 330 core

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

// These are game-defined uniforms. Renderer3D finds their values in the
// material parameters, then applies any per-draw overrides with matching names.
uniform vec3 effectTint;
uniform float effectStrength;
uniform sampler2D detailTexture;
uniform float detailStrength;

out vec4 fragmentColor;

void main() {
    vec4 sampledAlbedo = hasAlbedoTexture != 0
        ? texture(albedoTexture, vTexCoord)
        : vec4(1.0);
    vec4 albedo = sampledAlbedo * albedoColor;
    vec3 detail = texture(detailTexture, vTexCoord * 4.0).rgb;
    albedo.rgb *= mix(vec3(1.0), detail, clamp(detailStrength, 0.0, 1.0));
    albedo.rgb *= mix(
        vec3(1.0),
        effectTint,
        clamp(effectStrength, 0.0, 1.0)
    );

    if (litMaterial == 0) {
        fragmentColor = albedo;
        return;
    }

    vec3 normalDirection = normalize(vNormal);
    vec3 directionToLight = normalize(-lightDirection);
    float diffuse = max(dot(normalDirection, directionToLight), 0.0);
    diffuse *= mix(1.15, 0.85, roughness);
    diffuse *= mix(1.0, 0.85, metallic);
    vec3 lighting = vec3(0.15) + lightColor * lightIntensity * diffuse;
    fragmentColor = vec4(albedo.rgb * lighting, albedo.a);
}
