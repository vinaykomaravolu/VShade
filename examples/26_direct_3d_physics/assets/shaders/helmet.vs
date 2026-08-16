#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinate;
layout(location = 3) in vec4 tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;
out vec3 worldNormal;
out vec3 worldPosition;

void main() {
    vec4 positionInWorld = model * vec4(position, 1.0);
    uv = textureCoordinate;
    worldNormal = normalize(mat3(transpose(inverse(model))) * normal);
    worldPosition = positionInWorld.xyz;
    gl_Position = projection * view * positionInWorld;
}
