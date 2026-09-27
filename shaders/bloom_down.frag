#version 430 core
in vec2 uv;
out vec3 downsample;

uniform sampler2D srcTexture;
uniform vec2 srcTexelSize;
uniform int karisAverage;

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

// 13-tap downsample from Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare".
void main() {
    vec2 t = srcTexelSize;
    vec3 a = texture(srcTexture, uv + t * vec2(-2.0,  2.0)).rgb;
    vec3 b = texture(srcTexture, uv + t * vec2( 0.0,  2.0)).rgb;
    vec3 c = texture(srcTexture, uv + t * vec2( 2.0,  2.0)).rgb;
    vec3 d = texture(srcTexture, uv + t * vec2(-2.0,  0.0)).rgb;
    vec3 e = texture(srcTexture, uv).rgb;
    vec3 f = texture(srcTexture, uv + t * vec2( 2.0,  0.0)).rgb;
    vec3 g = texture(srcTexture, uv + t * vec2(-2.0, -2.0)).rgb;
    vec3 h = texture(srcTexture, uv + t * vec2( 0.0, -2.0)).rgb;
    vec3 i = texture(srcTexture, uv + t * vec2( 2.0, -2.0)).rgb;
    vec3 j = texture(srcTexture, uv + t * vec2(-1.0,  1.0)).rgb;
    vec3 k = texture(srcTexture, uv + t * vec2( 1.0,  1.0)).rgb;
    vec3 l = texture(srcTexture, uv + t * vec2(-1.0, -1.0)).rgb;
    vec3 m = texture(srcTexture, uv + t * vec2( 1.0, -1.0)).rgb;

    if (karisAverage == 1) {
        // Luma-weighted blocks keep single hot pixels (stars, the photon ring) from flickering in the bloom.
        vec3 g0 = (a + b + d + e) * 0.25;
        vec3 g1 = (b + c + e + f) * 0.25;
        vec3 g2 = (d + e + g + h) * 0.25;
        vec3 g3 = (e + f + h + i) * 0.25;
        vec3 g4 = (j + k + l + m) * 0.25;
        float w0 = 0.125 / (1.0 + luma(g0));
        float w1 = 0.125 / (1.0 + luma(g1));
        float w2 = 0.125 / (1.0 + luma(g2));
        float w3 = 0.125 / (1.0 + luma(g3));
        float w4 = 0.5 / (1.0 + luma(g4));
        downsample = (g0 * w0 + g1 * w1 + g2 * w2 + g3 * w3 + g4 * w4) / (w0 + w1 + w2 + w3 + w4);
    } else {
        downsample = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    }
    downsample = max(downsample, vec3(0.0));
}
