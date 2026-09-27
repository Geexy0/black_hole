#version 430 core
in vec3 worldPos;
in float radius;
out vec4 FragColor;

uniform sampler2D sceneDistance;
uniform vec2 viewport;
uniform vec3 camPos;
uniform float rimRadius;

void main() {
    if (distance(worldPos, camPos) > texture(sceneDistance, gl_FragCoord.xy / viewport).r) discard;
    float fade = 1.0 - smoothstep(0.6 * rimRadius, rimRadius, radius);
    FragColor = vec4(0.45, 0.65, 1.0, 0.35 * fade);
}
