#version 460

const vec2 pos[4] = {
    {-1.f,  1.f},
    {-1.f, -1.f},
    { 1.f,  1.f},
    { 1.f, -1.f},
};

void main() {
    gl_Position = vec4(pos[gl_VertexIndex], 1, 1);
}