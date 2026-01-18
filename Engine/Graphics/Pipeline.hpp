#pragma once

#include "Graphics/VkContext.hpp"
#include "Graphics/PipelineLayout.hpp"
#include "Utility/PtrFactory.hpp"
#include <vector>

class ENGINE_API Pipeline {
public:
    explicit Pipeline(std::shared_ptr<PipelineLayout> pipelineLayout) : m_pipelineLayout(std::move(pipelineLayout)) {}
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;
    virtual ~Pipeline() = default;

    [[nodiscard]] virtual PipelineBindPoint GetBindPoint() const = 0;

    [[nodiscard]] FORCE_INLINE vk::Pipeline GetPipeline() const { return m_pipeline; }
    [[nodiscard]] FORCE_INLINE std::shared_ptr<PipelineLayout> GetPipelineLayout() const { return m_pipelineLayout; }
    [[nodiscard]] FORCE_INLINE virtual const std::vector<vk::DescriptorSet>& GetUsedDscSets() const { return m_usedDscSets; }

    Pipeline(const Pipeline &) = delete;
    Pipeline &operator=(const Pipeline &) = delete;
protected:
    vk::raii::Pipeline m_pipeline = VK_NULL_HANDLE;
    std::shared_ptr<PipelineLayout> m_pipelineLayout = nullptr;

    std::vector<vk::DescriptorSet> m_usedDscSets;
};

struct GraphicsPipelineCreateInfo {
    std::string_view vertexShader = "";
    std::string_view fragmentShader = "";
    std::string_view geometryShader = ""; 
    std::string_view tessellationControlShader = "";
    std::string_view tessellationEvaluationShader = "";

    bool depthTest = true;
    bool depthWrite = true;
    vk::CompareOp depthTestCompareOp = vk::CompareOp::eLessOrEqual;
    vk::Viewport viewport{};
    vk::Extent2D extent{};
    std::span<ImageFormat> formats{};
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone;
    vk::FrontFace frontFace = vk::FrontFace::eClockwise;
    std::span<vk::DynamicState> dynamicStates{};
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
};

class ENGINE_API GraphicsPipeline final : public Pipeline, public PtrFactory<GraphicsPipeline> {
public:
    explicit GraphicsPipeline(const std::shared_ptr<PipelineLayout>& pipelineLayout = nullptr);
    GraphicsPipeline(GraphicsPipeline &&) = delete;
    GraphicsPipeline &operator=(GraphicsPipeline &&) = delete;
    ~GraphicsPipeline() override = default;

    [[nodiscard]] PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::eGraphics; }

    GraphicsPipeline& SetVertexInputState(const Shader::StageInterface& vertexInputs);
    GraphicsPipeline& SetVertexInputState(const vk::VertexInputBindingDescription& bindingDesc, const std::vector<vk::VertexInputAttributeDescription>& attribDesc);
    GraphicsPipeline& SetViewportState(const vk::Viewport& viewport, vk::Rect2D scissor);
    GraphicsPipeline& SetDynamicStateInfo(std::span<vk::DynamicState> dynamicStates = {});
    GraphicsPipeline& SetInputAssamblyStateInfo(vk::PrimitiveTopology topology);
    GraphicsPipeline& SetMultisampleStateInfo();

    GraphicsPipeline& SetShaderStages(std::string_view vertexShader,
                                    std::string_view fragmentShader,
                                    std::string_view geometryShader = "",
                                    std::string_view tessellationControlShader = "",
                                    std::string_view tessellationEvaluationShader = "");

    GraphicsPipeline& SetResterStateInfo(vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone,
                                        vk::FrontFace frontFace = vk::FrontFace::eClockwise, 
                                        vk::PolygonMode polygonMode = vk::PolygonMode::eFill);

    GraphicsPipeline& SetRenderingCreateInfo(std::span<vk::Format> colorFormats, 
                                            vk::Format depthFormat = vk::Format::eUndefined, 
                                            vk::Format stencilFormat = vk::Format::eUndefined);

    GraphicsPipeline& SetDepthStencilState(bool depthTest = true, 
                                        bool depthWrite = true, 
                                        vk::CompareOp depthCompareOp = vk::CompareOp::eLessOrEqual, 
                                        bool stencilTest = false);

    void CreatePipeline();
    [[nodiscard]] bool IsCreated() const { return (*m_pipeline) != VK_NULL_HANDLE; }

    GraphicsPipeline(const GraphicsPipeline &) = delete;
    GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
private:
    Shader* vertexShader;
    Shader* fragmentShader;
    Shader* geometryShader;
    Shader* tessellationControlShader;
    Shader* tessellationEvaluationShader;

    vk::VertexInputBindingDescription m_bindingDesc{};
    std::vector<vk::VertexInputAttributeDescription> m_attributeDesc{};
    vk::PipelineVertexInputStateCreateInfo m_vertexInputInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStageInfo;
    std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
    vk::PipelineColorBlendStateCreateInfo m_colorBlendState;
    vk::PipelineRasterizationStateCreateInfo m_resterStateInfo;
    vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyStateInfo;
    vk::PipelineMultisampleStateCreateInfo m_multisampleStateInfo;
    vk::PipelineDepthStencilStateCreateInfo m_depthStencilStateInfo;
    vk::PipelineRenderingCreateInfo m_renderingCreateInfo;
    vk::PipelineDynamicStateCreateInfo m_dynamicStateInfo;
    vk::PipelineViewportStateCreateInfo m_viewportStateInfo;
    vk::Viewport m_viewport;
    vk::Rect2D m_scissor;
    std::span<vk::Format> m_format;
};

class ENGINE_API ComputePipeline final : public Pipeline, public PtrFactory<ComputePipeline> {
public:
    explicit ComputePipeline(std::string_view shader, const std::shared_ptr<PipelineLayout>& pipelineLayout = nullptr);
    ComputePipeline(ComputePipeline &&) = delete;
    ComputePipeline &operator=(ComputePipeline &&) = delete;
    ~ComputePipeline() override = default;

    [[nodiscard]] PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::eCompute; }

    ComputePipeline(const ComputePipeline &) = delete;
    ComputePipeline &operator=(const ComputePipeline &) = delete;
private:
    void CreatePipeline(const Shader* shader);
};