#include "Graphics/Shader.hpp"

#include "Graphics/Image.hpp"
#include "Graphics/Descriptor.hpp"
#include "Utility/Counter.hpp"
#include <Spirv-Reflect/spirv_reflect.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

static std::vector<uint32_t> LoadShaderFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        LogError("failed to open {}", filePath.string());

    const auto fileSize = file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t)); //SPIR-V consists of 32 bit words 

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    file.close();

    return buffer;
}

static uint32_t GetFormatSize(const vk::Format format) {
    switch (format) {
        case vk::Format::eR32Sfloat:               return 4;
        case vk::Format::eR32G32Sfloat:            return 8;
        case vk::Format::eR32G32B32Sfloat:         return 12;
        case vk::Format::eR32G32B32A32Sfloat:      return 16;

        case vk::Format::eR8Uint:                  return 4;
        case vk::Format::eR8G8Uint:                return 4;
        case vk::Format::eR8G8B8Uint:              return 4;
        case vk::Format::eR8G8B8A8Uint:            return 4;

        case vk::Format::eR32Sint:                 return 4;
        case vk::Format::eR32G32Sint:              return 8;
        case vk::Format::eR32G32B32Sint:           return 12;
        case vk::Format::eR32G32B32A32Sint:        return 16;

        case vk::Format::eR16G16B16A16Sfloat:       return 8;
        case vk::Format::eR16G16B16Sfloat: LogWarning("support is bad: {}", vk::to_string(format)); return 6;
        case vk::Format::eR16G16Sfloat:             return 4;
        case vk::Format::eR16Sfloat:                return 2;

        default: {
            LogWarning("indeterminate format {}", vk::to_string(format));
            return 0;
        }
    };
}

vk::VertexInputBindingDescription Shader::StageInterface::GetBindingDescription() const {
    if (variables.empty())
        return {};

    uint32_t stride = 0;

    for (const auto& var : variables)
        stride += var.size;

    return {0, stride};
}

std::vector<vk::VertexInputAttributeDescription> Shader::StageInterface::GetInputAttribDescription() const {
    std::vector<vk::VertexInputAttributeDescription> attribDesc(variables.size());

    uint32_t offset = 0;

    for (const size_t i : Counter(variables.size())) {
        attribDesc[i]
            .setBinding(0)
            .setFormat(variables[i].type)
            .setLocation(i)
            .setOffset(offset)
            ;
            
        offset += variables[i].size;
    };

    return attribDesc;
}

Shader::Shader(std::string_view name) : m_name(name) {
    auto shaderCode = LoadShaderFile("./Shaders/Bin/" + std::string(m_name) + ".spv");

    //create shader module
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCode(shaderCode);
    try {
        m_shaderModule = VkContext::Get().GetDevice().createShaderModule(createInfo);
    }
    catch (const std::exception& e) {
        LogError("{}", e.what());
    }
    //create shader reflection
    spv_reflect::ShaderModule reflect(shaderCode);
    m_stage = static_cast<vk::ShaderStageFlagBits>(reflect.GetShaderStage());

    std::set<uint8_t> usedDstSets;

    //bindings
    {
        uint32_t count;
        reflect.EnumerateDescriptorBindings(&count, nullptr);
    
        std::vector<SpvReflectDescriptorBinding *> bindings(count);
        reflect.EnumerateDescriptorBindings(&count, bindings.data());

        for (auto& binding : bindings) {
            m_usedDescriptorDescs.emplace_back(
                DescriptorDesc {
                    .stage = m_stage,
                    .descriptorCount = binding->count,
                    .type = static_cast<vk::DescriptorType>(binding->descriptor_type),
                    .location = {static_cast<uint8_t>(binding->set),static_cast<uint8_t>(binding->binding)},
                }
            );

            usedDstSets.emplace(static_cast<uint8_t>(binding->set));
        }
    }

    for (auto dstSet : usedDstSets)
        m_usedDescriptorSets.emplace_back(dstSet);

    //push constants
    {
        uint32_t count;
        reflect.EnumeratePushConstantBlocks(&count, nullptr);
    
        std::vector<SpvReflectBlockVariable*> pushConstants(count);
        reflect.EnumeratePushConstantBlocks(&count, pushConstants.data());

        if (count != 0)
            m_usedPushConstant = vk::PushConstantRange {
                m_stage,
                pushConstants[0]->offset,
                pushConstants[0]->size
            };
    }

    //input block
    {
        uint32_t count;
        reflect.EnumerateInputVariables(&count, nullptr);

        std::vector<SpvReflectInterfaceVariable*> inputVariables(count);
        reflect.EnumerateInputVariables(&count, inputVariables.data());

        if (m_stage == vk::ShaderStageFlagBits::eVertex) {
            for (auto* inputVar : inputVariables) {
                if (inputVar->location > 32)//for vertex stage inputs(gl_VertexIndex etc.)
                    continue;

                m_stageInterface.variables.emplace_back(
                Variable {
                    .location = inputVar->location,
                    .size = GetFormatSize(static_cast<vk::Format>(inputVar->format)),
                    .type = static_cast<vk::Format>(inputVar->format),
                });
            }

            //Sort by location from smallest to largests
            std::ranges::sort(m_stageInterface.variables, [](const Shader::Variable& a, const Shader::Variable& b) {
                return a.location < b.location;
            });
        }
    }

    //output block
    {
        uint32_t count;
        reflect.EnumerateOutputVariables(&count, nullptr);

        std::vector<SpvReflectInterfaceVariable*> outputVariables(count);
        reflect.EnumerateOutputVariables(&count, outputVariables.data());

        if (m_stage == vk::ShaderStageFlagBits::eFragment) {
            for (auto* outputVar : outputVariables) {
                m_stageInterface.variables.emplace_back(
                Variable{
                    .location = outputVar->location,
                    .size = GetFormatSize(static_cast<vk::Format>(outputVar->format)),
                    .type = static_cast<vk::Format>(outputVar->format),
                });
            }
        }
    }
}