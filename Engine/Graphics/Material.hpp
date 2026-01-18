#pragma once

#include "Graphics/Shader.hpp"
#include "Graphics/Texture.hpp"
#include <glm/glm.hpp>

constexpr DescriptorLocation MaterialDstLocation = {0, 2};

//Represents the index in the Material Array
constexpr uint32_t MaterialUndefined = -1;
using MaterialHandle = uint32_t;

//Metallic-Roughness
struct alignas(16) MaterialData {
    glm::vec3 albedoValue = {1.f, 1.f, 1.f};
    float metallicValue = 1.f;
    float roughnessValue = 1.f;

    ImageHandle albedoTex = ImageUndefined;
    ImageHandle metallicRoughnessTex = ImageUndefined;
    ImageHandle normalTex = ImageUndefined;
};

struct alignas(16) MaterialBuffer {
    uint32_t count;
    MaterialData array[32];
};

struct Material {
    MaterialData* data = nullptr;
    MaterialHandle handle = MaterialUndefined;
};