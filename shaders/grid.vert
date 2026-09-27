#version 430 core
layout(location = 0) in vec2 aXZ;

uniform mat4 viewProj;
uniform float depthScale;
uniform float rimY;
uniform float rimRadius;

out vec3 worldPos;
out float radius;

// Flamm's paraboloid z(r) = 2 sqrt(r - 1) (r_s = 1), hung from the rim and clamped at the horizon.
void main() {
    float r = length(aXZ);
    float y = rimY - depthScale * 2.0 * (sqrt(rimRadius - 1.0) - sqrt(max(r, 1.0) - 1.0));
    worldPos = vec3(aXZ.x, y, aXZ.y);
    radius = r;
    gl_Position = viewProj * vec4(worldPos, 1.0);
}
