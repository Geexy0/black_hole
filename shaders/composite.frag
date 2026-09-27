#version 430 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D screenTexture;

void main() {
    FragColor = texture(screenTexture, uv);
}
