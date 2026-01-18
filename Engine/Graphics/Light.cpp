#include "Graphics/Light.hpp"

#include "Graphics/Renderer.hpp"
#include "Engine.hpp"

DirectionalLight::DirectionalLight(glm::vec2 extent, const DirectionalLightData& lightData)
    : data(lightData), extent(extent) {
    dataPtr = &GetRenderer().GetDirectionalLightData(GetRenderer().CreateLight(data));

    ImageCreateInfo shadowMapCreateInfo;
    shadowMapCreateInfo.arraySize = 1;
    shadowMapCreateInfo.extent = {ShadowMapSize, ShadowMapSize, 1};
    shadowMapCreateInfo.format = ImageFormat::eR32G32B32A32Sfloat;
    shadowMapCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
    shadowMapCreateInfo.type = ImageType::e2D;
    shadowMapCreateInfo.mipmapCount = 2;
    shadowMapCreateInfo.usage = ImageUsageBit::eColorAttachment | ImageUsageBit::eSampled | ImageUsageBit::eStorage;

    image = Image::New(shadowMapCreateInfo);

    lightSpaceMatrixPtr = &dataPtr->lightSpaceMatrix;

    CalcLightSpaceMtx();
    dataPtr->shadowMapIndex = GetRenderer().AddShadowMap(*this);
}

void DirectionalLight::CalcLightSpaceMtx() {
    glm::mat4 projectionMtx = glm::ortho(-extent.x/2.f, extent.x/2.f, -extent.y/2.f, extent.y/2.f, 0.1f, 1000.f);

    projectionMtx[1][1] *= -1;

    const glm::mat4 viewMtx = glm::lookAt(
        data.position,
        data.position + glm::normalize(data.direction),
        glm::vec3(0.0f, 1.0f, 0.0f));

    dataPtr->lightSpaceMatrix = projectionMtx * viewMtx;
}

PointLight::PointLight(const PointLightData& lightData)
    : data(lightData) {
    ImageCreateInfo shadowMapCreateInfo;
    shadowMapCreateInfo.arraySize = 1;
    shadowMapCreateInfo.extent = {1024, 1024, 1};
    shadowMapCreateInfo.format = ImageFormat::eD32Sfloat;
    shadowMapCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
    shadowMapCreateInfo.type = ImageType::eCube;
    shadowMapCreateInfo.usage = ImageUsageBit::eDepthAttachment | ImageUsageBit::eSampled;
    shadowMapCreateInfo.arraySize = 6;

    image = Image::New(shadowMapCreateInfo);

    CalcLightSpaceMtx();
    LogError("this class is not complated");
}

void PointLight::CalcLightSpaceMtx() {
    //CalcLightSpaceMtx();
}

SpotLight::SpotLight(const SpotLightData& lightData)
    : data(lightData) {
    ImageCreateInfo shadowMapCreateInfo;
    shadowMapCreateInfo.arraySize = 1;
    shadowMapCreateInfo.extent = {1024, 1024, 1};
    shadowMapCreateInfo.format = ImageFormat::eD32Sfloat;
    shadowMapCreateInfo.memoryUsage = MemoryType::eDeviceLocal;
    shadowMapCreateInfo.type = ImageType::e2D;
    shadowMapCreateInfo.usage = ImageUsageBit::eDepthAttachment | ImageUsageBit::eSampled;

    image = Image::New(shadowMapCreateInfo);

    CalcLightSpaceMtx();
}

void SpotLight::CalcLightSpaceMtx() {
    glm::mat4 projectionMtx = glm::perspective(glm::radians(90.f), 1.f, 1.f, 1000.f);

    projectionMtx[1][1] *= -1;

    const glm::mat4 viewMtx = glm::lookAt(
        data.position, 
        data.position + glm::normalize(data.direction),
        glm::vec3(0.0f, 1.0f, 0.0f));
    dataPtr->lightSpaceMatrix = projectionMtx * viewMtx;
}