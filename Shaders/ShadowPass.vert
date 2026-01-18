#version 460

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

#ifdef InputOutputfp16Supported
    #extension GL_EXT_shader_explicit_arithmetic_types_float16 : require

    #define iovec2 f16vec2
    #define iovec3 f16vec3
    #define iovec4 f16vec4
    #define iomat4 f16mat4
    #define iomat3 f16mat3
#else
    #define iovec2 vec2
    #define iovec3 vec3
    #define iovec4 vec4
    #define iomat4 mat4
    #define iomat3 mat3
#endif

#define BufferPtr uvec2

layout(location = 0) in iovec4 in_position;//f16vec3 support is bad

layout(push_constant) uniform pc {
    mat4 lightMtx;
    BufferPtr submesh_buffer_ptr;
};

struct InstanceData {
    mat4 model_mtx;
};

struct DrawData {
    uvec2 instance_buffer_handle;
    uint material_handle;
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly InstanceBuffer {
    InstanceData instance_data;
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly SubmeshDataBuffer {
    uint material_handle; uint padding;
    BufferPtr instance_buffer_ptr_array[];
};

layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly SubmeshBuffer {
    BufferPtr submesh_ptr_array[];
};

void main() {
    BufferPtr submesh_ptr = SubmeshBuffer(submesh_buffer_ptr).submesh_ptr_array[gl_DrawID];
    BufferPtr instance_data_ptr = SubmeshDataBuffer(submesh_ptr).instance_buffer_ptr_array[gl_InstanceIndex];
    InstanceData instance_data = InstanceBuffer(instance_data_ptr).instance_data;

    gl_Position = lightMtx * instance_data.model_mtx * vec4(vec3(in_position), 1);
}