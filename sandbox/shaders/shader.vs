#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureCoordinate;

out vec2 vertexTextureCoordinate;

void main() {
    vertexTextureCoordinate = textureCoordinate;
    gl_Position = vec4(position, 1.0);
}
