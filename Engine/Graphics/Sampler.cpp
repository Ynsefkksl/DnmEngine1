#include "Graphics/Sampler.hpp"

Sampler::Sampler(const vk::Filter filter, const vk::SamplerAddressMode addressMode, const vk::CompareOp compareOp) {
    const auto device = *VkContext::Get().GetDevice();
    const auto physicalDevice = VkContext::Get().GetPhysicalDevice();

    const auto properties = physicalDevice.getProperties();
    const auto features = physicalDevice.getFeatures();

    vk::SamplerCreateInfo createInfo;
    createInfo.setMinFilter(filter)
                .setMagFilter(filter)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.f)
                .setAddressModeU(addressMode)
                .setAddressModeV(addressMode)
                .setAddressModeW(addressMode)
                .setAnisotropyEnable(vk::True)
                .setMaxAnisotropy(4)
                .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
                .setUnnormalizedCoordinates(vk::False)
                .setCompareEnable((compareOp == vk::CompareOp::eAlways || compareOp == vk::CompareOp::eNever) ? vk::False : vk::True)
                .setCompareOp(compareOp)
                .setMinLod(0)
                .setMaxLod(VK_LOD_CLAMP_NONE)
                ;
    
    try {
        m_sampler = device.createSampler(createInfo);
    } catch (const std::exception& e) {
        LogError("{}", e.what());
    }
}

Sampler::~Sampler() {
    const auto device = *VkContext::Get().GetDevice();
    device.waitIdle();

    if (m_sampler != VK_NULL_HANDLE)
        device.destroySampler(m_sampler);
}