#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textureCoordinate;

uniform mat4 viewProjection;
uniform mat4 model;

out vec3 vertexNormal;
out vec2 vertexTextureCoordinate;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vertexNormal = normalize(normalMatrix * normal);
    vertexTextureCoordinate = textureCoordinate;
    gl_Position = viewProjection * model * vec4(position, 1.0);
}
