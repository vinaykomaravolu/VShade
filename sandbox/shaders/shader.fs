#version 330 core

in vec2 vertexTextureCoordinate;

uniform sampler2D image;
out vec4 fragmentColor;

void main() {
    fragmentColor = texture(image, vertexTextureCoordinate);
}
