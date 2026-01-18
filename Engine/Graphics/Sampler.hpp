#pragma once

#include "Graphics/VkContext.hpp"

class Sampler {
public:
    Sampler(vk::Filter filter, vk::SamplerAddressMode addressMode, vk::CompareOp compareOp);
    ~Sampler();

    Sampler(const Sampler& other) = delete;
    Sampler(Sampler&& other) = delete;
    Sampler& operator=(const Sampler& other) = delete;
    Sampler& operator=(Sampler&& other) = delete;

    [[nodiscard]] vk::Sampler GetSampler() const { return m_sampler; }
private:
    vk::Sampler m_sampler = VK_NULL_HANDLE;
};