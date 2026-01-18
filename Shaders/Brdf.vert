#version 450

vec2 pos[] = {
    {-1.f,  1.f},
    {-1.f, -1.f},
    { 1.f,  1.f},
    { 1.f, -1.f},
};

vec2 uv[] = {
    {0.f, 1.f},
    {0.f, 0.f},
    {1.f, 1.f},
    {1.f, 0.f},
};

layout(location = 0) out vec2 texCoord;

void main() {
    texCoord = uv[gl_VertexIndex];
    gl_Position = vec4(pos[gl_VertexIndex], 0, 1);
}