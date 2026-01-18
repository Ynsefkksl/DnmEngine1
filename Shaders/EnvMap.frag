#version 460
layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 in_pos;

layout(set = 0, binding = 1) uniform samplerCube envMap;

vec3 PBRNeutralToneMapping(vec3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1. - startCompression;
    float newPeak = 1. - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return mix(color, newPeak * vec3(1, 1, 1), g);
}

void main() {
    fragColor = vec4(textureLod(envMap, normalize(in_pos), 0).rgb, 1.f);

    fragColor.rgb = pow(fragColor.rgb, vec3(1.f/2.2));
}