#pragma once

#include "Graphics/VkContext.hpp"

class PipelineLayout;

class ENGINE_API Shader {
public:
    struct Variable {
        uint32_t location;
        uint32_t size;
        vk::Format type;
    };

    struct StageInterface {
        std::vector<Variable> variables{};

        [[nodiscard]] vk::VertexInputBindingDescription GetBindingDescription() const;
        [[nodiscard]] std::vector<vk::VertexInputAttributeDescription> GetInputAttribDescription() const;
    };

    Shader(std::string_view name);
    ~Shader() = default;

    [[nodiscard]] FORCE_INLINE vk::ShaderStageFlagBits GetStage() const noexcept { return m_stage; }
    [[nodiscard]] FORCE_INLINE vk::ShaderModule GetShaderModule() const noexcept { return m_shaderModule; }

    [[nodiscard]] FORCE_INLINE const std::vector<DescriptorDesc>& GetUsedDescriptorDescs() const noexcept { return m_usedDescriptorDescs; }
    [[nodiscard]] FORCE_INLINE const std::vector<uint8_t>& GetUsedDescriptorSets() const noexcept { return m_usedDescriptorSets; }//for utility
    [[nodiscard]] FORCE_INLINE const std::optional<vk::PushConstantRange>& GetUsedPushConstant() const noexcept { return m_usedPushConstant; }
    // Holds inputs for the vertex stage and outputs for the fragment stage.
    [[nodiscard]] FORCE_INLINE const StageInterface& GetStageInterface() const noexcept { return m_stageInterface; }

    [[nodiscard]] FORCE_INLINE std::string_view GetName() const { return m_name; }

    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&) = delete;
private:
    std::string m_name;

    vk::raii::ShaderModule m_shaderModule{ VK_NULL_HANDLE };

    vk::ShaderStageFlagBits m_stage = vk::ShaderStageFlagBits::eAll;

    std::vector<DescriptorDesc> m_usedDescriptorDescs;
    std::vector<uint8_t> m_usedDescriptorSets;//for utility
    std::optional<vk::PushConstantRange> m_usedPushConstant;

    // Holds inputs for the vertex stage and outputs for the fragment stage.
    StageInterface m_stageInterface;
};
