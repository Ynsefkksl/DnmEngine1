#version 460

#ifdef InputOutputfp16Supported
    #extension GL_EXT_shader_explicit_arithmetic_types_float16 : require

    #define fp float16_t
    #define fp2 f16vec2
    #define fp3 f16vec3
    #define fp4 f16vec4
    #define mtx4 f16mat4
    #define mtx3 f16mat3
#else
    #define fp float
    #define fp2 vec2
    #define fp3 vec3
    #define fp4 vec4
    #define mtx4 mat4
    #define mtx3 mat3
#endif

const uint Undefined_Image = 0;
const uint Undefined_Material = -1;
const uint Undefined_Light = -1;

const uint MaxMaterialCount = 32;
const uint MaxTextureCount = 128;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;

    uint albedoTex;
    uint metallicRoughnessTex; //b component metallic, g component roughness
    uint normalTex;
};

layout(location = 0) in in_block {
    mtx3 TBN;
    fp3 pos;
    fp3 normal;
    fp2 tex_coord;
    flat uint materialHandle;
} _in;

layout(location = 0) out fp4 outAlbedoRoughness;
layout(location = 1) out fp4 outNormalMetallic;
layout(location = 2) out fp4 outPosition;

layout(set = 0, binding = 1) uniform sampler2D textures[MaxTextureCount];

layout(set = 0, binding = 2) uniform material_buffer {
    uint count;
    Material array[MaxMaterialCount];
} materials;

void main() {
    const Material usedMaterial = materials.array[_in.materialHandle];
    const vec3 albedo = texture(textures[usedMaterial.albedoTex], _in.tex_coord).rgb * usedMaterial.albedo;

    vec3 normal = (texture(textures[usedMaterial.normalTex], _in.tex_coord).rgb * 2.0) - 1;
    normal = normalize(_in.TBN * normal);

    const vec2 metallicRoughness
        = texture(textures[usedMaterial.metallicRoughnessTex], _in.tex_coord).bg;

    const float metallic = metallicRoughness.x * usedMaterial.metallic;
    const float roughness = metallicRoughness.y  * usedMaterial.roughness;

    outAlbedoRoughness = fp4(albedo, roughness);
    outNormalMetallic = fp4(normal, metallic);
    outPosition.xyz = fp3(_in.pos);
}