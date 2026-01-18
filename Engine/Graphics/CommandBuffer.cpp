#include "Graphics/CommandBuffer.hpp"

#include <vk_mem_alloc.h>

#include "Graphics/RawBuffer.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/Image.hpp"
#include "Utility/Counter.hpp"

void CommandBuffer::CopyImageToBuffer(const Image& srcImage, const RawBuffer& dstBuffer, const BufferImageCopyInfo& info) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

    const vk::ImageSubresourceLayers subResource(
            srcImage.GetAspect(), // base aspect
            info.subResource.baseMipmapLevel, // base mipmap
            info.subResource.baseLayer, // base layer
            info.subResource.layerCount // layer count
        );

    vk::BufferImageCopy2 bufferImageCopy(
            info.bufferOffset, // buffer offset
            0, // buffer row lenght
            0, // buffer image height
            subResource, // imageSubresource
            info.imageOffset, // imageOffset
            {static_cast<uint32_t>(info.imageExtent.x), // image extent
                static_cast<uint32_t>(info.imageExtent.y),
                static_cast<uint32_t>(info.imageExtent.z)}
            );

    const vk::CopyImageToBufferInfo2 vkInfo(
        srcImage.GetImage(),// srcImage
        srcImage.GetImageLayout(),// imageLayout
        dstBuffer.GetBuffer(),// dstBuffer
        { bufferImageCopy }// regions
        );

    cmdBuffer.copyImageToBuffer2(vkInfo);
}

void CommandBuffer::CopyImageToImage(const Image& srcImage, const Image& dstImage, const ImageToImageCopyInfo& info) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

    const vk::ImageSubresourceLayers srcSubResource(
            srcImage.GetAspect(), // base aspect
            info.srcSubResource.baseMipmapLevel, // base mipmap
            info.srcSubResource.baseLayer, // base layer
            info.srcSubResource.layerCount // layer count
        );

    const vk::ImageSubresourceLayers dstSubResource(
            dstImage.GetAspect(), // base aspect
            info.dstSubResource.baseMipmapLevel, // base mipmap
            info.dstSubResource.baseLayer, // base layer
            info.dstSubResource.layerCount // layer count
        );

    vk::ImageCopy2 imageCopy(
        srcSubResource, // subresource
        {static_cast<int32_t>(info.srcOffset.x), // image offset
                static_cast<int32_t>(info.srcOffset.y),
                static_cast<int32_t>(info.srcOffset.z)},
        dstSubResource, // subresource
        {static_cast<int32_t>(info.dstOffset.x), // image offset
                static_cast<int32_t>(info.dstOffset.y),
                static_cast<int32_t>(info.dstOffset.z)},
        {static_cast<uint32_t>(info.extent.x), // image extent
                static_cast<uint32_t>(info.extent.y),
                static_cast<uint32_t>(info.extent.z)}
    );

    const vk::CopyImageInfo2 vkInfo(
        srcImage.GetImage(), // src image
        srcImage.GetImageLayout(), // src image layout
        dstImage.GetImage(), // dst image
        dstImage.GetImageLayout(), // dst image layout
        {imageCopy} // region
        );

    cmdBuffer.copyImage2(vkInfo);
}

void CommandBuffer::CopyBufferToBuffer(const RawBuffer& srcBuffer, const RawBuffer& dstBuffer, const BufferToBufferCopyInfo& info) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

    vk::BufferCopy2 bufferCopy(
        info.srcOffset, // src offset
        info.dstOffset, // dst offset
        info.size // size
    );

    const vk::CopyBufferInfo2 vkInfo(
        srcBuffer.GetBuffer(), // src buffer
        dstBuffer.GetBuffer(), // dst buffer
        {bufferCopy} // region
        );

    cmdBuffer.copyBuffer2(vkInfo);
}

void CommandBuffer::CopyBufferToImage(const RawBuffer& srcBuffer, const Image& dstImage, const BufferImageCopyInfo& info) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

    const vk::ImageSubresourceLayers subResource(
            dstImage.GetAspect(), // base aspect
            info.subResource.baseMipmapLevel, // base mipmap
            info.subResource.baseLayer, // base layer
            info.subResource.layerCount // layer count
        );

    vk::BufferImageCopy2 bufferImageCopy(
        info.bufferOffset, // buffer offset
        0, // buffer row lenght
        0, // buffer image height
        subResource, // imageSubresource
        info.imageOffset, // imageOffset
        {static_cast<uint32_t>(info.imageExtent.x), // image extent
            static_cast<uint32_t>(info.imageExtent.y),
            static_cast<uint32_t>(info.imageExtent.z)}
        );

    const vk::CopyBufferToImageInfo2 vkInfo(
        srcBuffer.GetBuffer(), // src buffer
        dstImage.GetImage(), // dst image
        dstImage.GetImageLayout(), // dst image layout
        {bufferImageCopy} // region
        );

    cmdBuffer.copyBufferToImage2(vkInfo);
}

void CommandBuffer::PipelineBarrier(std::span<PipelineBarrierInfo> memoryBarriers) const {
    //any queue
    //typeFlagBit |= 0;

    std::vector<vk::MemoryBarrier2> vkMemoryBarriers(memoryBarriers.size());

    uint32_t i = 0;
    for (auto& barrier : vkMemoryBarriers) {
        barrier = vk::MemoryBarrier2(
                *reinterpret_cast<vk::PipelineStageFlags2*>(&memoryBarriers[i].srcStageFlags), // src stage mask
                *reinterpret_cast<vk::AccessFlags2*>(&memoryBarriers[i].srcAccessFlags), // src access mask
                *reinterpret_cast<vk::PipelineStageFlags2*>(&memoryBarriers[i].dstStageFlags), // dst stage mask
                *reinterpret_cast<vk::AccessFlags2*>(&memoryBarriers[i].dstAccessFlags) // dst access mask
                );
        ++i;
    }

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        vkMemoryBarriers, // global memory barriers
        {}, // buffer memory barriers
        {} // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::TransferBufferQueue(RawBuffer& buffer, const uint32_t newFamilyIndex) const {
    //any queue
    //typeFlagBit |= 0;

    if (buffer.GetQueueFamily() == newFamilyIndex) return;

    vk::BufferMemoryBarrier2 memoryBarrier(
        {}, // src stage mask
        {}, // src access mask
        {}, // dst stage mask
        {}, // dst access mask
        buffer.GetQueueFamily(), // src queue family
        newFamilyIndex, // dst queue family
        buffer.GetBuffer(), // buffer
        0, // offset
        VK_WHOLE_SIZE // size
        );

    buffer.m_queueFamilyIndex = newFamilyIndex;

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        { memoryBarrier }, // buffer memory barriers
        {} // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::TransferImageLayoutAndQueue(Image& image, const vk::ImageLayout newImageLayout, const uint32_t newFamilyIndex) const {
    //any queue
    //typeFlagBit |= 0;

    if (image.GetQueueFamily() == newFamilyIndex) return;

    vk::ImageMemoryBarrier2 memoryBarrier(
        {}, // src stage mask
        {}, // src access mask
        {}, // dst stage mask
        {}, // dst access mask
        image.GetImageLayout(), // src image Layout
        newImageLayout, // dst image Layout
        image.GetQueueFamily(), // src queue family
        newFamilyIndex, // dst queue family
        image.GetImage(), // image
        vk::ImageSubresourceRange( // sub resource
                image.GetAspect(), // aspect
                0, // base mipmap
                VK_REMAINING_MIP_LEVELS, // mipmap count
                0, // base layer
                VK_REMAINING_ARRAY_LAYERS // layer count
            ));

    image.m_queueFamilyIndex = newFamilyIndex;
    image.m_imageLayout = newImageLayout;

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        {}, // buffer memory barriers
        {memoryBarrier} // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::TransferImageLayoutAndQueue(
            const vk::Image image,
            const vk::ImageAspectFlags aspect,
            const vk::ImageLayout oldImageLayout,
            const vk::ImageLayout newImageLayout,
            const uint32_t oldFamilyIndex,
            const uint32_t newFamilyIndex) const {
    //any queue
    //typeFlagBit |= 0;

    vk::ImageMemoryBarrier2 memoryBarrier(
        {}, // src stage mask
        {}, // src access mask
        {}, // dst stage mask
        {}, // dst access mask
        oldImageLayout, // src image Layout
        newImageLayout, // dst image Layout
        oldFamilyIndex, // src queue family
        newFamilyIndex, // dst queue family
        image, // image
        vk::ImageSubresourceRange( // sub resource
                aspect, // aspect
                0, // base mipmap
                VK_REMAINING_MIP_LEVELS, // mipmap count
                0, // base layer
                VK_REMAINING_ARRAY_LAYERS // layer count
            ));

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        {}, // buffer memory barriers
        {memoryBarrier} // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::BlitImage(const Image& srcImage, const Image& dstImage, const ImageBlitInfo& blitInfo) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;

    vk::ImageBlit2 imageBlit(
        vk::ImageSubresourceLayers(
            srcImage.GetAspect(),
            blitInfo.srcSubResource.baseMipmapLevel,
            blitInfo.srcSubResource.baseLayer,
            blitInfo.srcSubResource.layerCount
            ), // src subresource

            {vk::Offset3D{
                static_cast<int>(blitInfo.srcOffset.x),
                static_cast<int>(blitInfo.srcOffset.y),
                0
            },
            vk::Offset3D{
                static_cast<int>(blitInfo.srcExtent.x),
                static_cast<int>(blitInfo.srcExtent.y),
                1
            }},// src image offset

            vk::ImageSubresourceLayers(
            srcImage.GetAspect(),
            blitInfo.dstSubResource.baseMipmapLevel,
            blitInfo.dstSubResource.baseLayer,
            blitInfo.dstSubResource.layerCount
            ), // dst subresource

            {vk::Offset3D{
                static_cast<int>(blitInfo.dstOffset.x),
                static_cast<int>(blitInfo.dstOffset.y),
                0
            },
            vk::Offset3D{
                static_cast<int>(blitInfo.dstExtent.x),
                static_cast<int>(blitInfo.dstExtent.y),
                1
            }}// dst image offset
        );

    vk::BlitImageInfo2 vkBlitInfo(
        srcImage.GetImage(), // src image
        srcImage.GetImageLayout(), // src image layout
        dstImage.GetImage(), // dst image layout
        dstImage.GetImageLayout(), // dst image layout
        {imageBlit}, // region
        vk::Filter::eLinear // filter
        );

    cmdBuffer.blitImage2({vkBlitInfo});
}

void CommandBuffer::GenerateMipMaps(const Image& image) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;

    PipelineBarrierInfo memoryBarrier{};
    for (uint32_t arrayIndex = 0; arrayIndex < image.GetArraySize(); arrayIndex++) {
        int32_t mipWidth = image.GetExtent().x;
        int32_t mipHeight = image.GetExtent().y;

        for (uint32_t mipmapIndex = 1; mipmapIndex < image.GetMipmapCount(); mipmapIndex++) {
            memoryBarrier.srcAccessFlags = AccessBit::eTransferRead;
            memoryBarrier.dstAccessFlags = AccessBit::eTransferWrite;
            memoryBarrier.srcStageFlags = PipelineStageBit::eTransfer;
            memoryBarrier.dstStageFlags = PipelineStageBit::eBlit;

            PipelineBarrier({&memoryBarrier, 1});

            ImageBlitInfo imageBlitInfo {
                .srcSubResource = ImageSubResource {
                    .baseMipmapLevel = mipmapIndex - 1,
                    .baseLayer = arrayIndex,
                    .mipmapLevelCount = 1,
                    .layerCount = 1,
                },
                .dstSubResource = ImageSubResource {
                    .baseMipmapLevel = mipmapIndex,
                    .baseLayer = arrayIndex,
                    .mipmapLevelCount = 1,
                    .layerCount = 1,
                },
                .srcExtent = {mipWidth, mipHeight},
                .dstExtent = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1},
            };

            BlitImage(image, image, imageBlitInfo);

            memoryBarrier.srcAccessFlags = AccessBit::eTransferWrite;
            memoryBarrier.dstAccessFlags = AccessBit::eShaderRead;
            memoryBarrier.srcStageFlags = PipelineStageBit::eBlit;
            memoryBarrier.dstStageFlags = PipelineStageBit::eFragmentShader;

            PipelineBarrier({&memoryBarrier, 1});

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        memoryBarrier.srcAccessFlags = AccessBit::eTransferWrite;
        memoryBarrier.dstAccessFlags = AccessBit::eShaderRead;
        memoryBarrier.srcStageFlags = PipelineStageBit::eBlit;
        memoryBarrier.dstStageFlags = PipelineStageBit::eFragmentShader;

        PipelineBarrier({&memoryBarrier, 1});
    }
}

void CommandBuffer::BeginRenderPass(RenderPass& renderPass, const glm::vec2& extent, const glm::vec2& offset) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;

    const vk::RenderingInfo info(
        // most likely this command will be called in the secondary cmdBuffer
        {}, // flags
        vk::Rect2D{
            {static_cast<int>(offset.x), static_cast<int>(offset.y)},
            {static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y)}
        }, // rendering area
        1, // layer count
        {}, // view mask
        renderPass.GetColorAttachments().size(),
        renderPass.GetColorAttachments().data(),
        renderPass.GetDepthAttachment().has_value() ? &renderPass.GetDepthAttachment().value() : nullptr,
        {} // stencil attachment
    );

    cmdBuffer.beginRendering(info);
}

void CommandBuffer::BeginRenderPass(const DynamicRenderPassInfo& info) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;

    std::vector<vk::RenderingAttachmentInfo> colorAttachments(info.colorAttachments.size());
    std::optional<vk::RenderingAttachmentInfo> depthAttachment{};

    uint32_t i = 0;
    for (auto& attachment : colorAttachments) {
        attachment = vk::RenderingAttachmentInfo(
            info.colorAttachments[i].imageView,
            info.colorAttachments[i].imageLayout,
            {},
            {},
            {},
            static_cast<vk::AttachmentLoadOp>(info.colorAttachments[i].loapOp),
            static_cast<vk::AttachmentStoreOp>(info.colorAttachments[i].storeOp),
            vk::ClearValue(vk::ClearColorValue{
                info.colorAttachments[i].clearValue.r,
                info.colorAttachments[i].clearValue.g,
                info.colorAttachments[i].clearValue.b,
                info.colorAttachments[i].clearValue.a})
        );
        i++;
    }

    if (info.depthAttachment.has_value())
        depthAttachment = vk::RenderingAttachmentInfo(
                info.depthAttachment->imageView,
                info.depthAttachment->imageLayout,
                {},
                {},
                {},
                static_cast<vk::AttachmentLoadOp>(info.depthAttachment->loapOp),
                static_cast<vk::AttachmentStoreOp>(info.depthAttachment->storeOp),
                vk::ClearValue(vk::ClearDepthStencilValue{
                    info.depthAttachment->clearValue.depthClear,
                    info.depthAttachment->clearValue.stencilClear
                })
            );

    const vk::RenderingInfo vkInfo(
        // most likely this command will be called in the secondary cmdBuffer
        {}, // flags
        vk::Rect2D{
            {static_cast<int>(info.offset.x), static_cast<int>(info.offset.y)},
            {static_cast<uint32_t>(info.extent.x), static_cast<uint32_t>(info.extent.y)}
        }, // rendering area
        1, // layer count
        {}, // view mask
        colorAttachments.size(),
        colorAttachments.data(),
        depthAttachment.has_value() ? &depthAttachment.value() : nullptr,
        {} // stencil attachment
    );

    cmdBuffer.beginRendering(vkInfo);
}

void CommandBuffer::EndRenderPass() {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;

    cmdBuffer.endRendering();
}

void CommandBuffer::BeginFrameOptimalFramebufferLayout(const Swapchain& swapchain) const {
    //any queue
    //typeFlagBit |= 0;

    constexpr vk::ImageMemoryBarrier2 memory_barrier(
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src stage mask
        vk::AccessFlagBits2::eMemoryRead, // src access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dst stage mask
        vk::AccessFlagBits2::eColorAttachmentWrite, // dst access mask
        vk::ImageLayout::ePresentSrcKHR, // src image Layout
        vk::ImageLayout::eColorAttachmentOptimal, // dst image Layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family
        VK_QUEUE_FAMILY_IGNORED, // dst queue family
        nullptr, // image
        vk::ImageSubresourceRange( // sub resource
                vk::ImageAspectFlagBits::eColor, // aspect
                0, // base mipmap
                1, // mipmap count
                0, // base layer
                1 // layer count
            ));

    std::vector<vk::ImageMemoryBarrier2> barriers(swapchain.GetImages().size());
    std::ranges::fill(barriers, memory_barrier);

    for (const auto i : Counter(swapchain.GetImages().size())) {
        barriers[i].image = swapchain.GetImages()[i];
    }

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        {}, // buffer memory barriers
        barriers // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::PresentFrameOptimalFramebufferLayout(const Swapchain& swapchain) const {
    //any queue
    //typeFlagBit |= 0;

    constexpr vk::ImageMemoryBarrier2 memory_barrier(
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src stage mask
        vk::AccessFlagBits2::eColorAttachmentWrite, // src access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dst stage mask
        vk::AccessFlagBits2::eMemoryRead, // dst access mask
        vk::ImageLayout::eColorAttachmentOptimal, // src image Layout
        vk::ImageLayout::ePresentSrcKHR, // dst image Layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family
        VK_QUEUE_FAMILY_IGNORED, // dst queue family
        nullptr, // image
        vk::ImageSubresourceRange( // sub resource
                vk::ImageAspectFlagBits::eColor, // aspect
                0, // base mipmap
                1, // mipmap count
                0, // base layer
                1 // layer count
            ));

    std::vector<vk::ImageMemoryBarrier2> barriers(swapchain.GetImages().size());
    std::ranges::fill(barriers, memory_barrier);

    for (const auto i : Counter(swapchain.GetImages().size())) {
        barriers[i].image = swapchain.GetImages()[i];
    }

    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        {}, // buffer memory barriers
        barriers // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::UploadData(RawBuffer& buffer, const void* data, const uint32_t size, const uint32_t offset) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

    if (size <= 65536) {
        (*cmdBuffer).updateBuffer(buffer.GetBuffer(), offset, size, data);
        return;
    }

    if (void* mappedPtr = buffer.MapMemory(); mappedPtr != nullptr) {
        memcpy(mappedPtr, data, size);
        buffer.UnmapMemory();
        return;
    };

    if (stagingBuffer == nullptr || stagingBuffer->GetSize() < size) {
        stagingBuffer = RawBuffer::NewUnique(
                size,
                vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
                MemoryType::eCpuReadWrite);
    }

    auto* mappedPtr = stagingBuffer->MapMemory();
        memcpy(mappedPtr, data, size);
    stagingBuffer->UnmapMemory();

    CopyBufferToBuffer(*stagingBuffer, buffer, BufferToBufferCopyInfo{
        .srcOffset = 0,
        .dstOffset = offset,
        .size = size
    });
}

void CommandBuffer::UploadData(const Image& image, const ImageSubResource& subResource, const void* data, const uint32_t size, const uint32_t offset) {
    typeFlagBit |= vk::QueueFlagBits::eTransfer;

     if (stagingBuffer == nullptr || stagingBuffer->GetSize() < size) {
        stagingBuffer = RawBuffer::NewUnique(
                size,
                vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
                MemoryType::eCpuReadWrite);
    }

    UploadData(*stagingBuffer, data, size);

    CopyBufferToImage(*stagingBuffer, image, BufferImageCopyInfo {
        .subResource = subResource,
        .imageExtent = image.GetExtent(),
        .bufferOffset = 0,
        .imageOffset = offset,
    });
}

void CommandBuffer::OptimalSwapchainImageLayout(const Swapchain& swapchain, const uint32_t image_index) const {
    const auto next_image_index = (swapchain.GetImages().size() == 1 + image_index) ? 
                                                        0 : image_index + 1;

    const auto current_image = swapchain.GetImages()[image_index];
    const auto next_image = swapchain.GetImages()[next_image_index];

    const vk::ImageMemoryBarrier2 current_barrier(
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src stage mask
        vk::AccessFlagBits2::eColorAttachmentWrite, // src access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dst stage mask
        {}, // dst access mask
        vk::ImageLayout::eColorAttachmentOptimal, // src image Layout
        vk::ImageLayout::ePresentSrcKHR, // dst image Layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family
        VK_QUEUE_FAMILY_IGNORED, // dst queue family
        current_image, // image
        vk::ImageSubresourceRange( // sub resource
                vk::ImageAspectFlagBits::eColor, // aspect
                0, // base mipmap
                1, // mipmap count
                0, // base layer
                1 // layer count
            ));

    const vk::ImageMemoryBarrier2 next_barrier(
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // src stage mask
        {}, // src access mask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dst stage mask
        vk::AccessFlagBits2::eColorAttachmentWrite, // dst access mask
        vk::ImageLayout::eUndefined, // src image Layout
        vk::ImageLayout::eColorAttachmentOptimal, // dst image Layout
        VK_QUEUE_FAMILY_IGNORED, // src queue family
        VK_QUEUE_FAMILY_IGNORED, // dst queue family
        next_image, // image
        vk::ImageSubresourceRange( // sub resource
                vk::ImageAspectFlagBits::eColor, // aspect
                0, // base mipmap
                1, // mipmap count
                0, // base layer
                1 // layer count
            ));

    const vk::ImageMemoryBarrier2 barriers[2] = {current_barrier, next_barrier};
    
    const vk::DependencyInfo dependencyInfo(
        {}, // dependencyFlags
        {}, // global memory barriers
        {}, // buffer memory barriers
        barriers // image memory barriers
        );

    cmdBuffer.pipelineBarrier2(dependencyInfo);
}
