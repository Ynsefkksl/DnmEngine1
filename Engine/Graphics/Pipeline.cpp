#include "Graphics/Pipeline.hpp"

#include "Graphics/Renderer.hpp"
#include "Graphics/VkContext.hpp"
#include "Utility/Counter.hpp"

#include <cstdint>
#include <ranges>
#include <set>

GraphicsPipeline& GraphicsPipeline::SetVertexInputState(const Shader::StageInterface& vertexInputs) {
    {
        m_bindingDesc = vertexInputs.GetBindingDescription();
        m_attributeDesc = vertexInputs.GetInputAttribDescription();
    }

    m_vertexInputInfo.setVertexAttributeDescriptions(m_attributeDesc)
                    .setVertexBindingDescriptions(m_bindingDesc)
                    ;

    if (m_attributeDesc.empty()) {
        m_vertexInputInfo.setPVertexAttributeDescriptions(nullptr)
                        .setPVertexBindingDescriptions(nullptr)
                        .setVertexAttributeDescriptionCount(0)
                        .setVertexBindingDescriptionCount(0)
                        ;
    }

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetVertexInputState(const vk::VertexInputBindingDescription& bindingDesc, const std::vector<vk::VertexInputAttributeDescription>& attribDesc) {
    {
        m_bindingDesc = bindingDesc;
        m_attributeDesc = attribDesc;
    }

    m_vertexInputInfo.setVertexAttributeDescriptions(m_attributeDesc)
                    .setVertexBindingDescriptions(m_bindingDesc)
                    ;

    if (m_attributeDesc.empty()) {
        m_vertexInputInfo.setPVertexAttributeDescriptions(nullptr)
                        .setPVertexBindingDescriptions(nullptr)
                        .setVertexAttributeDescriptionCount(0)
                        .setVertexBindingDescriptionCount(0)
                        ;
    }

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetShaderStages(std::string_view vertexShaderName,
                                                std::string_view fragmentShaderName,
                                                std::string_view geometryShaderName,
                                                std::string_view tessellationControlShaderName,
                                                std::string_view tessellationEvaluationShaderName) {

    if (m_pipelineLayout == nullptr) {
        std::vector<std::string_view> shaders;
        shaders.emplace_back(vertexShaderName);
        shaders.emplace_back(fragmentShaderName);
        if (!geometryShaderName.empty()) shaders.emplace_back(geometryShaderName);
        if (!tessellationControlShaderName.empty()) shaders.emplace_back(tessellationControlShaderName);
        if (!tessellationEvaluationShaderName.empty()) shaders.emplace_back(tessellationEvaluationShaderName);
        m_pipelineLayout = PipelineLayout::New(shaders);
    }

    vertexShader = m_pipelineLayout->GetShader(vertexShaderName);
    fragmentShader = m_pipelineLayout->GetShader(fragmentShaderName);
    geometryShader = m_pipelineLayout->GetShader(geometryShaderName);
    tessellationControlShader = m_pipelineLayout->GetShader(tessellationControlShaderName);
    tessellationEvaluationShader = m_pipelineLayout->GetShader(tessellationEvaluationShaderName);

    if (vertexShader->GetStage() != vk::ShaderStageFlagBits::eVertex)
        LogError("vertex shader stage mismatch: actual stage is {}", vk::to_string(vertexShader->GetStage()));

    if (fragmentShader->GetStage() != vk::ShaderStageFlagBits::eFragment)
        LogError("fragment shader stage mismatch: actual stage is {}", vk::to_string(fragmentShader->GetStage()));

    if (geometryShader != nullptr)
        if (geometryShader->GetStage() != vk::ShaderStageFlagBits::eGeometry)
            LogError("geometry shader stage mismatch: actual stage is {}", vk::to_string(geometryShader->GetStage()));
      
    if (tessellationControlShader != nullptr)
        if (tessellationControlShader->GetStage() != vk::ShaderStageFlagBits::eTessellationControl)
            LogError("tessControl shader stage mismatch: actual stage is {}", vk::to_string(tessellationControlShader->GetStage()));

    if (tessellationEvaluationShader != nullptr)
        if (tessellationEvaluationShader->GetStage() != vk::ShaderStageFlagBits::eTessellationEvaluation)
            LogError("tessEval shader stage mismatch: actual stage is {}", vk::to_string(tessellationEvaluationShader->GetStage()));

    // "^" is xor 
    if ((tessellationControlShader == nullptr) ^ (tessellationEvaluationShader == nullptr))
        LogError("Both tessellation shaders must be present or absent together.");

    m_shaderStageInfo.emplace_back(
        vk::PipelineShaderStageCreateInfo (
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertexShader->GetShaderModule(),
            "main",
            nullptr
        )
    );
    m_shaderStageInfo.emplace_back(
        vk::PipelineShaderStageCreateInfo (
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragmentShader->GetShaderModule(),
            "main",
            nullptr
        )
    );
    if (geometryShader != nullptr)
        m_shaderStageInfo.emplace_back(
            vk::PipelineShaderStageCreateInfo (
                {},
                vk::ShaderStageFlagBits::eGeometry,
                geometryShader->GetShaderModule(),
                "main",
                nullptr
            )
        );
    if (tessellationControlShader != nullptr)
        m_shaderStageInfo.emplace_back(
            vk::PipelineShaderStageCreateInfo (
                {},
                vk::ShaderStageFlagBits::eTessellationControl,
                tessellationControlShader->GetShaderModule(),
                "main",
                nullptr
            )
        );
    if (tessellationEvaluationShader != nullptr)
        m_shaderStageInfo.emplace_back(
            vk::PipelineShaderStageCreateInfo (
                {},
                vk::ShaderStageFlagBits::eTessellationEvaluation,
                tessellationEvaluationShader->GetShaderModule(),
                "main",
                nullptr
            )
        );

    std::set<vk::DescriptorSet> usedDstSets{};

    for (auto dstSet : vertexShader->GetUsedDescriptorSets())
        usedDstSets.insert(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());

    for (auto dstSet : fragmentShader->GetUsedDescriptorSets())
        usedDstSets.insert(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());

    if (geometryShader != nullptr)
        for (auto dstSet : geometryShader->GetUsedDescriptorSets())
            usedDstSets.insert(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());

    if (tessellationControlShader != nullptr)
        for (auto dstSet : tessellationControlShader->GetUsedDescriptorSets())
            usedDstSets.insert(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());

    if (tessellationEvaluationShader != nullptr)
        for (auto dstSet : tessellationEvaluationShader->GetUsedDescriptorSets())
            usedDstSets.insert(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());

    for (auto& dstSet : usedDstSets)
        m_usedDscSets.emplace_back(dstSet);

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetResterStateInfo(        
        vk::CullModeFlags cullMode,
        vk::FrontFace frontFace, 
        vk::PolygonMode polygonMode) {
    m_resterStateInfo.setDepthClampEnable(vk::False)
                    .setPolygonMode(polygonMode)
                    .setLineWidth(1.f)
                    .setCullMode(cullMode)
                    .setFrontFace(frontFace)
                    ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetViewportState(const vk::Viewport& viewport, const vk::Rect2D scissor) {
    m_viewport = viewport;

    m_scissor = scissor;

    m_viewportStateInfo.setViewports(m_viewport)
                        .setScissors(m_scissor)
                        ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetDepthStencilState(bool depthTest,
                                                bool depthWrite,
                                                vk::CompareOp depthCompareOp, 
                                                bool stencilTest) {
    m_depthStencilStateInfo.setDepthTestEnable(depthTest)
                            .setDepthWriteEnable(depthWrite)
                            .setDepthCompareOp(depthCompareOp)
                            .setStencilTestEnable(stencilTest)
                            ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetDynamicStateInfo(std::span<vk::DynamicState> dynamicStates) {
    m_dynamicStateInfo.setDynamicStates(dynamicStates)
                        ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetInputAssamblyStateInfo(vk::PrimitiveTopology topology) {
    m_inputAssemblyStateInfo.setPrimitiveRestartEnable(vk::False)
                            .setTopology(topology)
                            ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetMultisampleStateInfo() {
    m_multisampleStateInfo.setSampleShadingEnable(vk::False)
                            .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                            .setMinSampleShading(1.f)
                            ;

    return *this;
}

GraphicsPipeline& GraphicsPipeline::SetRenderingCreateInfo(const std::span<vk::Format> colorFormats, const vk::Format depthFormat, const vk::Format stencilFormat) {
    m_format = colorFormats;

    m_renderingCreateInfo.setColorAttachmentFormats(m_format)
                        .setDepthAttachmentFormat(depthFormat)
                        .setStencilAttachmentFormat(stencilFormat)
                        .setViewMask(0)
                        ;

    for ([[maybe_unused]] auto i : Counter(m_renderingCreateInfo.colorAttachmentCount)) {
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.setColorWriteMask(
                            vk::ColorComponentFlagBits::eR | 
                            vk::ColorComponentFlagBits::eG | 
                            vk::ColorComponentFlagBits::eB | 
                            vk::ColorComponentFlagBits::eA)
                            .setBlendEnable(vk::False);
                            
        m_colorBlendAttachments.emplace_back(colorBlendAttachment);
    }

    m_colorBlendState.setAttachments(m_colorBlendAttachments)
                                .setLogicOpEnable(vk::False);

    return *this;
}

GraphicsPipeline::GraphicsPipeline(const std::shared_ptr<PipelineLayout>& pipelineLayout) : Pipeline(pipelineLayout),
    vertexShader(nullptr),
    fragmentShader(nullptr),
    geometryShader(nullptr),
    tessellationControlShader(nullptr),
    tessellationEvaluationShader(nullptr) {}

void GraphicsPipeline::CreatePipeline() {
    if (IsCreated()) {
        LogWarning("this pipeline created");
        return;
    };

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.setStages(m_shaderStageInfo)
                        .setPVertexInputState(&m_vertexInputInfo)
                        .setPInputAssemblyState(&m_inputAssemblyStateInfo)
                        .setPRasterizationState(&m_resterStateInfo)
                        .setPMultisampleState(&m_multisampleStateInfo)
                        .setPDepthStencilState(&m_depthStencilStateInfo)
                        .setPColorBlendState(&m_colorBlendState)
                        .setPViewportState(&m_viewportStateInfo)
                        .setPDynamicState(&m_dynamicStateInfo)
                        .setLayout(m_pipelineLayout->GetPipelineLayout())
                        .setPNext(&m_renderingCreateInfo)
                        ;

    try {
        m_pipeline = VkContext::Get().GetDevice().createGraphicsPipeline(nullptr, pipelineCreateInfo);
    }
    catch (const std::exception& e) {
        LogError("{}", e.what());
    }
}

ComputePipeline::ComputePipeline(const std::string_view shader, const std::shared_ptr<PipelineLayout>& pipelineLayout) : Pipeline(pipelineLayout) {
    if (m_pipelineLayout == nullptr) {
        std::vector shaderNameArray{shader}; //constructer problem
        m_pipelineLayout = PipelineLayout::New(std::span(shaderNameArray));
    }
    try {
        CreatePipeline(m_pipelineLayout->GetShader(shader));
    }
    catch (const std::exception& e) {
        LogError("{}", e.what());
    }
}

void ComputePipeline::CreatePipeline(const Shader* shader) {
    vk::PipelineShaderStageCreateInfo shaderStageInfo;
    shaderStageInfo.setStage(vk::ShaderStageFlagBits::eCompute)
                    .setModule(shader->GetShaderModule())
                    .setPName("main")
                    ;

    vk::ComputePipelineCreateInfo createInfo;
    createInfo.setLayout(m_pipelineLayout->GetPipelineLayout())
                .setStage(shaderStageInfo)
                ;

    m_pipeline = VkContext::Get().GetDevice().createComputePipeline(nullptr, createInfo);

    for (const auto dstSet : shader->GetUsedDescriptorSets())
        m_usedDscSets.emplace_back(m_pipelineLayout->GetDescriptorSet(dstSet)->GetDescriptorSet());
}