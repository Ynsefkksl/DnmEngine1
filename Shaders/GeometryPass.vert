#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

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

#define BufferPtr uvec2

layout(location = 0) in fp4 in_position;//f16vec3 support is bad
layout(location = 1) in fp4 in_tangent;
layout(location = 2) in fp4 in_normal;//f16vec3 support is bad
layout(location = 3) in fp2 in_tex_coord;

struct InstanceData {
    mat4 model_mtx;
};

layout(set = 0, binding = 0) uniform camera_ubo {
    mat4 p;
    mat4 v;
} camera;

layout(push_constant) uniform pc {
    BufferPtr submesh_buffer_ptr;
};

layout(location = 0) out out_block {
    mtx3 TBN;
    fp3 pos;
    fp3 normal;
    fp2 tex_coord;
    flat uint material_handle;
} _out;

layout(buffer_reference, std430, buffer_reference_align = 16) buffer readonly InstanceBuffer {
    InstanceData instance_data;
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly SubmeshDataBuffer {
    uint material_handle; uint padding;
    BufferPtr instance_buffer_ptr_array[];
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly restrict SubmeshBuffer {
    BufferPtr submesh_ptr_array[];
};

void main() {
    const BufferPtr submesh_ptr = SubmeshBuffer(submesh_buffer_ptr).submesh_ptr_array[gl_DrawID];
    const BufferPtr instance_data_ptr = SubmeshDataBuffer(submesh_ptr).instance_buffer_ptr_array[gl_InstanceIndex];
    const InstanceData instance_data = InstanceBuffer(instance_data_ptr).instance_data;
    _out.material_handle = SubmeshDataBuffer(submesh_ptr).material_handle;

    const fp3 bitangent = cross(in_normal.xyz, in_tangent.xyz) * in_tangent.w;
    const mtx4 model_mtx = mtx4(instance_data.model_mtx);

    fp3 T = normalize(fp3(model_mtx * in_tangent));
    const fp3 B = normalize(fp3(model_mtx * fp4(bitangent, 0.0)));
    const fp3 N = normalize(fp3(model_mtx * in_normal));
    T = normalize(T - dot(T, N) * N);
    _out.TBN = mtx3(T, B, N);

    _out.normal = fp3(transpose(inverse(mtx3(model_mtx))) * in_normal.xyz);

    _out.pos = fp3(model_mtx * fp4(in_position.xyz, 1));
    gl_Position = camera.p * camera.v * vec4(_out.pos, 1);

    _out.tex_coord = in_tex_coord;
}