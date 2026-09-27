#version 430 core
in vec2 uv;
out vec3 upsample;

uniform sampler2D srcTexture;
uniform vec2 srcTexelSize;

// 3x3 tent filter; the result is added onto the next larger mip with additive blending.
void main() {
    vec2 t = srcTexelSize;
    vec3 a = texture(srcTexture, uv + t * vec2(-1.0,  1.0)).rgb;
    vec3 b = texture(srcTexture, uv + t * vec2( 0.0,  1.0)).rgb;
    vec3 c = texture(srcTexture, uv + t * vec2( 1.0,  1.0)).rgb;
    vec3 d = texture(srcTexture, uv + t * vec2(-1.0,  0.0)).rgb;
    vec3 e = texture(srcTexture, uv).rgb;
    vec3 f = texture(srcTexture, uv + t * vec2( 1.0,  0.0)).rgb;
    vec3 g = texture(srcTexture, uv + t * vec2(-1.0, -1.0)).rgb;
    vec3 h = texture(srcTexture, uv + t * vec2( 0.0, -1.0)).rgb;
    vec3 i = texture(srcTexture, uv + t * vec2( 1.0, -1.0)).rgb;
    upsample = (e * 4.0 + (b + d + f + h) * 2.0 + (a + c + g + i)) / 16.0;
}
