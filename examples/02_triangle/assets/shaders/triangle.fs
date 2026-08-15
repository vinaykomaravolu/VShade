#version 330 core
in vec2 uv;
out vec4 fragmentColor;
void main() {
    fragmentColor = vec4(uv.x, uv.y, 1.0 - uv.x, 1.0);
}
