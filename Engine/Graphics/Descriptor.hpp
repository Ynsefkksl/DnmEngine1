#pragma once

#include "Graphics/RawBuffer.hpp"
#include "Graphics/Texture.hpp"
#include <variant>
#include <vector>

//forward declaration
class PipelineLayout;
class DescriptorSet;
namespace spv_reflect {
    class ShaderModule;
};

class Descriptor {
public:
    explicit Descriptor(const DescriptorDesc& desc);
    ~Descriptor() = default;

    [[nodiscard]] FORCE_INLINE vk::DescriptorSetLayoutBinding GetBinding() const {
        return {
            m_location.binding,
            m_type,
            m_descriptorCount,
            m_stage,
            nullptr
        };
    }

    void SetResource(const std::vector<RawBuffer*>& bufferArray, uint32_t arrayIndex) const;
    void SetResource(const std::vector<ImageResource>& imageArray, uint32_t arrayIndex) const;

    FORCE_INLINE void AddStage(const vk::ShaderStageFlags stage) noexcept { m_stage |= stage; }

    [[nodiscard]] FORCE_INLINE DescriptorSet* GetDescriptorSet() const noexcept { return m_descriptorSet; }
    [[nodiscard]] FORCE_INLINE DescriptorLocation GetLocation() const noexcept { return m_location; }
    [[nodiscard]] FORCE_INLINE vk::ShaderStageFlags GetStage() const noexcept { return m_stage; }

    Descriptor(Descriptor&&) = delete;
    Descriptor(Descriptor&) = delete;
    Descriptor& operator=(Descriptor&&) = delete;
    Descriptor& operator=(Descriptor&) = delete;
private:
    DescriptorLocation m_location;
    vk::DescriptorType m_type;
    uint32_t m_descriptorCount = 0;
    vk::ShaderStageFlags m_stage = vk::ShaderStageFlagBits::eAll;

    DescriptorSet* m_descriptorSet = nullptr;

    friend std::vector<Descriptor*> DescriptorCreation(spv_reflect::ShaderModule& reflect, vk::ShaderStageFlags stage);
    friend DescriptorSet;
    friend PipelineLayout;
    friend Shader;
};

class DescriptorSet {
public:
    explicit DescriptorSet(const uint8_t index) : index(index) {}
    ~DescriptorSet() = default;

    FORCE_INLINE void AddDescriptor(Descriptor* descriptor) noexcept { m_descriptors.emplace_back(descriptor); }

    [[nodiscard]] std::vector<vk::DescriptorSetLayoutBinding> GetBindings();

    void CreateDescriptorSetLayout();
    void CreateDescriptorSet(vk::DescriptorPool dstPool);

    [[nodiscard]] FORCE_INLINE uint8_t GetIndex() const noexcept { return index; }
    [[nodiscard]] FORCE_INLINE const std::vector<Descriptor*>& GetDescriptors() const noexcept { return m_descriptors; }
    [[nodiscard]] FORCE_INLINE vk::DescriptorSet GetDescriptorSet() const noexcept { return descriptorSet; }
    [[nodiscard]] FORCE_INLINE vk::DescriptorSetLayout GetDescriptorLayout() const noexcept { return descriptorSetLayout; }

    DescriptorSet(const DescriptorSet&) = delete;
    DescriptorSet(DescriptorSet&&) = delete;
    DescriptorSet& operator=(const DescriptorSet&) = delete;
    DescriptorSet& operator=(DescriptorSet&&) = delete;
private:
    uint8_t index = UndefinedLocationValue;
    std::vector<Descriptor*> m_descriptors{};
    vk::raii::DescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    vk::DescriptorSet descriptorSet = VK_NULL_HANDLE;
};