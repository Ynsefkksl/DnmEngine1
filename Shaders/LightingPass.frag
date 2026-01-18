#version 460

const uint albedoRoughnessIndex = 0;
const uint normalMetallicIndex = 1;
const uint positionIndex = 2;

const uint MaxDirectionalLightCount = 8;
const uint MaxSpotLightCount = 8;
const uint MaxPointLightCount = 32;

const float PI = 3.14159265359;

const float c = 120;

struct DirectionalLight {
    mat4 lightSpaceMatrix;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    uint shadowMapIndex;
};

struct SpotLight {
    mat4 lightSpaceMatrix;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float innerConeAngle;
    float outerConeAngle;
    uint shadowMapIndex;
};

struct PointLight {
    mat4 lightSpaceMatrix;
    vec3 position;
    vec3 color;
    float intensity;
    float range;
    uint shadowMapIndex;
};

layout(location = 0) out vec4 fragColor;

layout(rgba16f, set = 0, binding = 0) readonly uniform image2DArray gBuffer;

layout(set = 0, binding = 1) uniform light_buffer {
    uint directionalLightCount;
    uint pointLightCount;
    uint spotLightCount;

    DirectionalLight directionalLightArray[MaxDirectionalLightCount];
    PointLight pointLightArray[MaxPointLightCount];
    SpotLight spotLightArray[MaxSpotLightCount];
} lights;

layout(set = 0, binding = 2) uniform samplerCube irradianceMap;
layout(set = 0, binding = 3) uniform samplerCube preFilterMap;
layout(set = 0, binding = 4) uniform sampler2D lutTexture;

layout(set = 0, binding = 5) uniform sampler2D shadowMaps[MaxDirectionalLightCount * MaxSpotLightCount];
layout(set = 0, binding = 6) uniform samplerCubeShadow pointLightShadowMaps[MaxPointLightCount];

layout(set = 0, binding = 7) uniform camera_ubo {
    vec4 camera_pos;
};

vec3 position = imageLoad(gBuffer, ivec3(gl_FragCoord.xy, positionIndex)).xyz;

float ChebyshevUpperBound(vec2 moments, float t) {
    float p = float(t <= moments.x);

    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.000001);

    float d = t - moments.x;
    float p_max = variance / (variance + d * d);
    return max(p, p_max);
}

float ShadowCalculation(vec4 fragPosLightSpace, uint mapIndex, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    const vec4 texValue = texture(shadowMaps[mapIndex], projCoords.xy, 0).rgba;

    const float posWarp = exp(c * projCoords.z);
    const float negWarp = -exp(-c * projCoords.z);

    const float posShadow = ChebyshevUpperBound(texValue.xy, posWarp);
    const float negShadow = ChebyshevUpperBound(texValue.zw, negWarp);

    return smoothstep(0.3, 1.0, min(posShadow, negShadow));
}

float PointLightShadowCalculation(vec4 fragPosLightSpace, uint mapIndex, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;

    vec2 texelSize = 1.0 / textureSize(pointLightShadowMaps[mapIndex], 0).xy;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            shadow += texture(pointLightShadowMaps[mapIndex], vec4(projCoords.xy + vec2(x, y) * texelSize, projCoords.z, 1), bias);
        }
    }
    shadow /= 9.0;
    
    return 0;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH; 

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

vec3 PBRNeutralToneMapping(vec3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    const float x = min(color.r, min(color.g, color.b));
    const float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    const float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1. - startCompression;
    const float newPeak = 1. - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    const float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return mix(color, newPeak * vec3(1, 1, 1), g);
}

vec3 CalcPointLight(PointLight light, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 normal, vec3 F0) {
    vec3 L = normalize(light.position - position);
    vec3 H = normalize(viewDir + L);
    float distance = length(light.position - position);
    float attenuation = max( min( 1.0 - pow(( distance / light.range ), 4), 1 ), 0 ) / pow(distance, 2);
    vec3 radiance = light.color * attenuation * light.intensity; //light color * attenuation * intensity

    float NDF = DistributionGGX(normal, H, roughness);   
    float G   = GeometrySmith(normal, viewDir, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;

    vec3 kD = vec3(1.0) - kS;

    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, L), 0.0);

    float shadow = PointLightShadowCalculation(light.lightSpaceMatrix * vec4(position, 1), light.shadowMapIndex, normal, L);

    return (kD * albedo / PI + specular) * radiance * NdotL * shadow;
}

vec3 CalcDirectionalLight(DirectionalLight light, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 normal, vec3 F0) {
    vec3 L = normalize(light.position - position);
    vec3 H = normalize(viewDir + L);
    float distance = length(light.position - position);
    vec3 radiance = light.color * light.intensity; //attenuation 1

    float NDF = DistributionGGX(normal, H, roughness);   
    float G   = GeometrySmith(normal, viewDir, L, roughness);      
    vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;    
    vec3 kS = F;

    vec3 kD = vec3(1.0) - kS;

    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, L), 0.0);

    float shadow = ShadowCalculation(light.lightSpaceMatrix * vec4(position, 1), light.shadowMapIndex, normal, L);

    return (kD * albedo / PI + specular) * radiance * NdotL * shadow;
}

vec3 CalcSpotLight(SpotLight light, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 normal, vec3 F0) {
    vec3 lightDir = normalize(light.position - position);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = (light.innerConeAngle - light.outerConeAngle);
    float intensity = clamp((theta - light.outerConeAngle) / epsilon, 0.0, 1.0);

    vec3 L = normalize(light.position - position);
    vec3 H = normalize(viewDir + L);
    float distance = length(light.position - position);
    float attenuation = max( min( 1.0 - pow(( distance / light.range ), 4), 1 ), 0 ) / pow(distance, 2);
    vec3 radiance = light.color * attenuation * light.intensity * intensity; //light color * attenuation * intensity

    float NDF = DistributionGGX(normal, H, roughness);   
    float G   = GeometrySmith(normal, viewDir, L, roughness);      
    vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;

    vec3 kD = vec3(1.0) - kS;

    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, L), 0.0);

    float shadow = ShadowCalculation(light.lightSpaceMatrix * vec4(position, 1), light.shadowMapIndex, normal, L);

    return (kD * albedo / PI + specular) * radiance * NdotL * shadow;
}

void main() {
    const vec4 albedoRoughness = imageLoad(gBuffer, ivec3(gl_FragCoord.xy, albedoRoughnessIndex)); 
    const vec3 albedo = albedoRoughness.rgb;
    const float roughness = albedoRoughness.a;

    const vec4 normalMetallic = imageLoad(gBuffer, ivec3(gl_FragCoord.xy, normalMetallicIndex));
    const vec3 normal = normalMetallic.rgb;
    const float metallic = normalMetallic.a;

    const vec3 viewDir = normalize(camera_pos.xyz - position);

    const vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0);
    uint lightCount = 0;

    //point light
    lightCount = min(lights.pointLightCount, MaxPointLightCount);
    for (uint i = 0; i < lightCount; i++)
        Lo += CalcPointLight(lights.pointLightArray[i], viewDir, albedo, metallic, roughness, normal, F0);

    //spot light
    lightCount = min(lights.spotLightCount, MaxSpotLightCount);
    for (uint i = 0; i < lightCount; i++)
        Lo += CalcSpotLight(lights.spotLightArray[i], viewDir, albedo, metallic, roughness, normal, F0);

    //directional light
    lightCount = min(lights.directionalLightCount, MaxDirectionalLightCount);
    for (uint i = 0; i < lightCount; i++)
        Lo += CalcDirectionalLight(lights.directionalLightArray[i], viewDir, albedo, metallic, roughness, normal, F0);

    const vec3 F = FresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
    
    const vec3 kS = F;
    const vec3 kD = (1.0 - kS) * (1.0 - metallic);

    const vec3 irradiance = texture(irradianceMap, normal).rgb;
    const vec3 diffuse = irradiance * albedo;

    const float maxReflectionLod = 4.0;

    const vec3 prefilteredColor = textureLod(preFilterMap, reflect(-viewDir, normal), roughness * maxReflectionLod).rgb;
    // Not sure why, but using texture() instead of textureLod(..., 0) gave huge performance gain (amd ryzen 5625u)
    const vec2 brdf = textureLod(lutTexture, vec2(max(dot(normal, viewDir), 0.f), roughness), 0).rg;
    const vec3 specular = prefilteredColor * (F * brdf.r + brdf.g);

    const vec3 ambient = (kD * diffuse + specular); /* * ao */

    vec3 color = ambient + Lo;

    color = PBRNeutralToneMapping(color);
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color, 1);
}
