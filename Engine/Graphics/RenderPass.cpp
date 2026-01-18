#include "Graphics/RenderPass.hpp"

#include "Graphics/Image.hpp"

void RenderPass::AddColorAttachment(const ColorAttachmentInfo& colorAttachmentInfo) {
    m_colorAttachments.emplace_back(
        vk::RenderingAttachmentInfo(
            colorAttachmentInfo.imageView,
            colorAttachmentInfo.imageLayout,
            {}, // resolve mode
            {}, // resolve image view
            {}, // resolve image layout
            static_cast<vk::AttachmentLoadOp>(colorAttachmentInfo.loapOp),
            static_cast<vk::AttachmentStoreOp>(colorAttachmentInfo.storeOp),
            vk::ClearValue(vk::ClearColorValue{
                colorAttachmentInfo.clearValue.r,
                colorAttachmentInfo.clearValue.g,
                colorAttachmentInfo.clearValue.b,
                colorAttachmentInfo.clearValue.a}
    )));
}

void RenderPass::SetDepthAttachment(const DepthAttachmentInfo& depthStencilAttachmentInfo) {
    m_depthAttachment.emplace(
        vk::RenderingAttachmentInfo(
            depthStencilAttachmentInfo.imageView,
            depthStencilAttachmentInfo.imageLayout,
            {}, // resolve mode
            {}, // resolve image view
            {}, // resolve image layout
            static_cast<vk::AttachmentLoadOp>(depthStencilAttachmentInfo.loapOp),
            static_cast<vk::AttachmentStoreOp>(depthStencilAttachmentInfo.storeOp),
            vk::ClearValue(vk::ClearDepthStencilValue{
                depthStencilAttachmentInfo.clearValue.depthClear,
                depthStencilAttachmentInfo.clearValue.stencilClear
            })
    ));
}