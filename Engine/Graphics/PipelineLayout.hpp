#pragma once

#include "Graphics/VkContext.hpp"

#include "Graphics/Descriptor.hpp"
#include "Graphics/Shader.hpp"

#include "Utility/PtrFactory.hpp"

class ENGINE_API PipelineLayout : public PtrFactory<PipelineLayout> {
public:
    explicit PipelineLayout(std::span<std::string_view> usedShaders);
    PipelineLayout(PipelineLayout &&) = delete;
    PipelineLayout &operator=(PipelineLayout &&) = delete;
    ~PipelineLayout() {
        m_descriptors.clear();
        m_descriptorSets.clear();
    }

    [[nodiscard]] vk::PipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
    [[nodiscard]] vk::DescriptorPool GetDescriptorPool() const { return m_descriptorPool; }

    [[nodiscard]] Descriptor* GetDescriptor(const DescriptorLocation location) const {
        const auto it = m_descriptors.find(location);
        return (it != m_descriptors.end()) ? it->second.get() : nullptr;
    }

    [[nodiscard]] DescriptorSet* GetDescriptorSet(const uint8_t index) const {
        const auto it = m_descriptorSets.find(index);
        return (it != m_descriptorSets.end()) ? it->second.get() : nullptr;
    }

    [[nodiscard]] Shader* GetShader(const std::string_view name) const {
        const auto it = m_shaders.find(std::string(name));
        return (it != m_shaders.end()) ? it->second : nullptr;
    }

    PipelineLayout(const PipelineLayout &) = delete;
    PipelineLayout &operator=(const PipelineLayout &) = delete;
private:
    void LoadShaders(std::span<std::string_view> usedShaders);
    void CreateDescriptorPool();
    Descriptor* RegisterDescriptor(DescriptorDesc desc);
    void CreateDescriptorSets();
    void CreatePipelineLayout();

    std::unordered_map<DescriptorLocation, std::unique_ptr<Descriptor>> m_descriptors;
    std::unordered_map<uint32_t, std::unique_ptr<DescriptorSet>> m_descriptorSets;
    std::unordered_map<std::string, Shader*> m_shaders;
    vk::raii::DescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    vk::raii::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

    friend Shader;
    friend Descriptor;
    friend DescriptorSet;
};