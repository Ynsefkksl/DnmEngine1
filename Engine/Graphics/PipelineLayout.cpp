#include "Graphics/PipelineLayout.hpp"

#include "Graphics/Shader.hpp"
#include "Graphics/Descriptor.hpp"
#include "Graphics/ShaderCache.hpp"

#include "Engine.hpp"

#include <filesystem>
#include <ranges>

PipelineLayout::PipelineLayout(const std::span<std::string_view> usedShaders) {
    LoadShaders(usedShaders);
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreatePipelineLayout();
}

void PipelineLayout::CreateDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolSizes{};
    poolSizes.emplace_back(vk::DescriptorType::eStorageBuffer, 64);
    poolSizes.emplace_back(vk::DescriptorType::eUniformBuffer, 64);
    //poolSizes.emplace_back(vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 64});
    //poolSizes.emplace_back(vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 64});
    poolSizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, 128);
    poolSizes.emplace_back(vk::DescriptorType::eStorageImage, 128);

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.setPoolSizes(poolSizes)
                .setMaxSets(32)
                .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);

    m_descriptorPool = VkContext::Get().GetDevice().createDescriptorPool(createInfo);
}

void PipelineLayout::LoadShaders(const std::span<std::string_view> usedShaders) {
    auto& shaderCache = Engine::Get().GetShaderCache();

    for (auto shaderName : usedShaders) {
        auto* shader = shaderCache.GetShader(shaderName);

        if (shader == nullptr)
            shader = shaderCache.RegisterShader(shaderName);

        m_shaders.try_emplace(std::string(shaderName), shader);

        for (auto& dstDesc : shader->GetUsedDescriptorDescs()) {
            RegisterDescriptor(dstDesc);
        }
    }
}

Descriptor* PipelineLayout::RegisterDescriptor(const DescriptorDesc desc) {
    auto [descriptorIt, istThere] = m_descriptors.try_emplace(desc.location, new Descriptor(desc));
    auto* descriptor = descriptorIt->second.get();

    if (!istThere) {
        descriptor->AddStage(desc.stage);
    }
    //if new created
    else {
        auto [descriptorSetIt, _] = m_descriptorSets.try_emplace(desc.location.set, new DescriptorSet(desc.location.set));
        descriptorSetIt->second->AddDescriptor(descriptor);
        descriptor->m_descriptorSet = descriptorSetIt->second.get();
    }

    return descriptor;
}

void PipelineLayout::CreateDescriptorSets() {
    for (const auto& dstSet : m_descriptorSets | std::views::values) {
        dstSet->CreateDescriptorSetLayout();
        dstSet->CreateDescriptorSet(m_descriptorPool);
    }
}

void PipelineLayout::CreatePipelineLayout() {
    std::vector<vk::DescriptorSetLayout> dstSetLayouts;
    std::vector<vk::PushConstantRange> pushConstants;

    //get layouts
    for (const auto& dstSet : m_descriptorSets | std::views::values)
        dstSetLayouts.emplace_back(dstSet->GetDescriptorLayout());

    //get push constants
    for (const auto& shader : m_shaders | std::views::values) {
        auto& pushConstant = shader->GetUsedPushConstant();
        if (!pushConstant.has_value()) continue;
        pushConstants.emplace_back(pushConstant.value());
    }

    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.setSetLayouts(dstSetLayouts);
    createInfo.setPushConstantRanges(pushConstants);

    m_pipelineLayout = VkContext::Get().GetDevice().createPipelineLayout(createInfo);
}