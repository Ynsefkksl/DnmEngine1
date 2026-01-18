#version 460

layout(location = 0) out vec4 outDepth;

const float c = 120;

void main() {
    const float depth = gl_FragCoord.z;

    const float tPositive = exp(c * depth);
    const float tNegative = -exp(-c * depth);

    outDepth.xy = vec2(tPositive, tPositive * tPositive);
    outDepth.zw = vec2(tNegative, tNegative * tNegative);
}