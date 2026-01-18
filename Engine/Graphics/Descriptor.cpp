#include "Graphics/Descriptor.hpp"

#include "Graphics/Texture.hpp"
#include "Graphics/Image.hpp"
#include "Graphics/Sampler.hpp"

Descriptor::Descriptor(const DescriptorDesc& desc)
    : m_location(desc.location), m_type(desc.type), m_descriptorCount(desc.descriptorCount)
        , m_stage(desc.stage)  {
    if (!(m_type == vk::DescriptorType::eCombinedImageSampler
        || m_type == vk::DescriptorType::eStorageImage
        || m_type == vk::DescriptorType::eStorageBuffer
        || m_type == vk::DescriptorType::eUniformBuffer))
        LogError("i dont support this descriptor type: {}", vk::to_string(m_type));
}

void Descriptor::SetResource(const std::vector<RawBuffer*>& bufferArray, const uint32_t arrayIndex) const {
    if (bufferArray.empty())
        return;
    if (!(m_type == vk::DescriptorType::eStorageBuffer || m_type == vk::DescriptorType::eUniformBuffer)) {
        LogWarning("wrong res type; descriptor: {}, descriptor type {}", static_cast<std::string>(m_location), vk::to_string((m_type)));
        return;
    }

    //update resource
    const auto device = *VkContext::Get().GetDevice();

    std::vector<vk::DescriptorBufferInfo> bufferInfos(bufferArray.size());

    uint32_t bufferIndex = 0;
    for (auto& buff : bufferArray)
        bufferInfos[bufferIndex++] = vk::DescriptorBufferInfo(
                buff->GetBuffer(),
                0,
                VK_WHOLE_SIZE);

    const vk::WriteDescriptorSet writeDescriptorSet(
        m_descriptorSet->GetDescriptorSet(),
        m_location.binding,
        arrayIndex,
        static_cast<uint32_t>(bufferInfos.size()),
        m_type,
        nullptr, // image info
        bufferInfos.data(), // buffer info
        nullptr // texel buffer info
    );

    device.updateDescriptorSets(writeDescriptorSet, {});
}

void Descriptor::SetResource(const std::vector<ImageResource>& imageArray, const uint32_t arrayIndex) const {
    if (imageArray.empty())
        return;

    if (!(m_type == vk::DescriptorType::eCombinedImageSampler || m_type == vk::DescriptorType::eStorageImage)) {
        LogWarning("wrong res type; descriptor: {}, descriptor type {}", static_cast<std::string>(m_location), vk::to_string((m_type)));
        return;
    }

    const auto device = *VkContext::Get().GetDevice();

    std::vector<vk::DescriptorImageInfo> imageInfo(imageArray.size());

    uint32_t imageIndex = 0;
    for (const auto& [subresource, image, sampler] : imageArray) {
        const auto imageView = image->CreateGetImageView(subresource);

        imageInfo[imageIndex++] =
            vk::DescriptorImageInfo{
                sampler == nullptr ? nullptr : sampler->GetSampler(),
                imageView,
                image->GetImageLayout()
            };
    }

    const vk::WriteDescriptorSet writeDescriptorSet(
        m_descriptorSet->GetDescriptorSet(),
        m_location.binding,
        arrayIndex,
        static_cast<uint32_t>(imageInfo.size()),
        m_type,
        imageInfo.data(), // buffer info
        nullptr, // image info
        nullptr // texel buffer info
    );

    device.updateDescriptorSets(writeDescriptorSet, {});
}

std::vector<vk::DescriptorSetLayoutBinding> DescriptorSet::GetBindings() {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    for (auto*& descriptor : m_descriptors)
        bindings.emplace_back(descriptor->GetBinding());
    return bindings;
}

void DescriptorSet::CreateDescriptorSetLayout() {
    auto layoutBindings = GetBindings();

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setBindings(layoutBindings)
                    .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
                    ;

    descriptorSetLayout = VkContext::Get().GetDevice().createDescriptorSetLayout(layoutCreateInfo);
}

void DescriptorSet::CreateDescriptorSet(const vk::DescriptorPool dstPool) {
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(dstPool)
            .setDescriptorSetCount(1)
            .setSetLayouts(*descriptorSetLayout)
            ;

    try {
        descriptorSet = (*VkContext::Get().GetDevice()).allocateDescriptorSets(allocInfo)[0];
    }
    catch (const std::exception& e) {
        LogError("{}", e.what());
    }
}