#pragma once

#include "Graphics/Shader.hpp"
#include <glm/glm.hpp>

//Represents the index in the Light Array
constexpr uint32_t LightUndefined = UINT32_MAX;
using LightHandle = uint32_t;

class ShadowMap {
public:
    virtual ~ShadowMap() = default;
    std::shared_ptr<Image> image;
    glm::mat4* lightSpaceMatrixPtr;

    virtual void CalcLightSpaceMtx() = 0;
};

struct alignas(16) DirectionalLightData {
    glm::mat4 lightSpaceMatrix = {};
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    float intensity;
    uint32_t shadowMapIndex;
};

struct alignas(16) SpotLightData {
    glm::mat4 lightSpaceMatrix;
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;
    float intensity;
    float range;
    float innerConeAngle = 0;
    float outerConeAngle = 0;
    uint32_t shadowMapIndex;
};

struct alignas(16) PointLightData {
    glm::mat4 lightSpaceMatrix;
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;
    float intensity;
    float range;
    uint32_t shadowMapIndex;// Unlike other shadowMapIndex values, this one points to an element in a samplerCubeShadow array
};

struct alignas(16) LightBuffer {
    uint32_t directionalLightCount = 0;
    uint32_t pointLightCount = 0;
    uint32_t spotLightCount = 0;
    
    DirectionalLightData directionalLightArray[8];
    PointLightData pointLightArray[32];
    SpotLightData spotLightArray[8];
};

class ENGINE_API DirectionalLight final : public ShadowMap {
public:
    DirectionalLight(glm::vec2 extent, const DirectionalLightData& lightData);

    void CalcLightSpaceMtx() override;
    
    DirectionalLightData data;
    DirectionalLightData* dataPtr;
    glm::vec2 extent;
};

class ENGINE_API PointLight final : public ShadowMap {
public:
    explicit PointLight(const PointLightData& lightData);

    void CalcLightSpaceMtx() override;
    
    PointLightData data;
    PointLightData* dataPtr;
};

class ENGINE_API SpotLight final : public ShadowMap {
public:
    explicit SpotLight(const SpotLightData& lightData);

    void CalcLightSpaceMtx() override;
    
    SpotLightData data;
    SpotLightData* dataPtr;
};