#version 330 core

in vec3 vertexNormal;
in vec2 vertexTextureCoordinate;

uniform sampler2D albedoTexture;
uniform int hasAlbedoTexture;
uniform int litMaterial;
uniform vec4 albedoColor;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform float lightIntensity;

out vec4 fragmentColor;

void main() {
    vec4 sampledAlbedo = hasAlbedoTexture != 0
        ? texture(albedoTexture, vertexTextureCoordinate)
        : vec4(1.0);
    vec4 albedo = sampledAlbedo * albedoColor;

    if (litMaterial == 0) {
        fragmentColor = albedo;
        return;
    }

    vec3 normalDirection = normalize(vertexNormal);
    vec3 directionToLight = normalize(-lightDirection);
    float diffuse = max(dot(normalDirection, directionToLight), 0.0);
    vec3 lighting = vec3(0.15) + lightColor * lightIntensity * diffuse;
    fragmentColor = vec4(albedo.rgb * lighting, albedo.a);
}
