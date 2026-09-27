#version 430 core
in vec2 uv;
out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float exposure;
uniform float bloomStrength;

// Narkowicz's fit of the ACES filmic curve.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    vec3 hdr = texture(sceneTexture, uv).rgb;
    vec3 bloom = texture(bloomTexture, uv).rgb;
    vec3 color = aces(mix(hdr, bloom, bloomStrength) * exposure);
    color = pow(color, vec3(1.0 / 2.2));
    color += (hash12(gl_FragCoord.xy) - 0.5) / 255.0;
    FragColor = vec4(color, 1.0);
}
