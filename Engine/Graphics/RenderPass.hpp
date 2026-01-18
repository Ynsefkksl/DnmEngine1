#pragma once

#include "Graphics/VkContext.hpp"

class RenderPass {
public:
    RenderPass() = default;
    ~RenderPass() = default;

    void AddColorAttachment(const ColorAttachmentInfo& colorAttachmentInfo);
    void SetDepthAttachment(const DepthAttachmentInfo& depthStencilAttachmentInfo);

    [[nodiscard]] std::vector<vk::RenderingAttachmentInfo>& GetColorAttachments() { return m_colorAttachments; }
    [[nodiscard]] std::optional<vk::RenderingAttachmentInfo>& GetDepthAttachment() { return m_depthAttachment; }
private:
    std::vector<vk::RenderingAttachmentInfo> m_colorAttachments;
    std::optional<vk::RenderingAttachmentInfo> m_depthAttachment;
};