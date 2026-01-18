#pragma once

#include "Swapchain.hpp"
#include "Graphics/VkContext.hpp"

#include "Utility/PtrFactory.hpp"

#include "Graphics/Pipeline.hpp"
#include "Graphics/PipelineLayout.hpp"
#include "Graphics/RawBuffer.hpp"

class CommandBuffer : public PtrFactory<CommandBuffer> {
public:
    explicit CommandBuffer(vk::raii::CommandBuffer& commandBuffer, const uint32_t queueFamilyIndex)
        : cmdBuffer(std::move(commandBuffer)), queueFamilyIndex(queueFamilyIndex) {}

    CommandBuffer(CommandBuffer&& other) noexcept
        : stagingBuffer(std::move(other.stagingBuffer)), cmdBuffer(std::move(other.cmdBuffer)),  queueFamilyIndex(other.queueFamilyIndex), typeFlagBit(other.typeFlagBit) {}

    CommandBuffer& operator=(CommandBuffer&& other) noexcept {
        if (&other != this) {
            cmdBuffer = std::move(other.cmdBuffer);
            stagingBuffer = std::move(other.stagingBuffer);
        }
        return *this;
    }

    ~CommandBuffer() = default;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    [[nodiscard]] vk::CommandBuffer GetCommandBuffer() const { return cmdBuffer; }

    void ExecuteCommandBuffer(const CommandBuffer& commandBuffer) const {
        cmdBuffer.executeCommands(commandBuffer.GetCommandBuffer());
        return;
    }

    void UploadData(const Image& image, const ImageSubResource& subResource, const void* data, uint32_t size, uint32_t offset = 0);
    void UploadData(RawBuffer& buffer, const void* data, uint32_t size, uint32_t offset = 0);

    void CopyImageToBuffer(const Image& srcImage, const RawBuffer& dstBuffer, const BufferImageCopyInfo& info);
    void CopyImageToImage(const Image& srcImage, const Image& dstImage, const ImageToImageCopyInfo& info);
    void CopyBufferToBuffer(const RawBuffer& srcBuffer, const RawBuffer& dstBuffer, const BufferToBufferCopyInfo& info);
    void CopyBufferToImage(const RawBuffer& srcBuffer, const Image& dstImage, const BufferImageCopyInfo& info);

    void PipelineBarrier(std::span<PipelineBarrierInfo> memoryBarriers) const;
    void TransferBufferQueue(RawBuffer& buffer, uint32_t newFamilyIndex) const;
    void TransferImageLayoutAndQueue(Image& image, vk::ImageLayout newImageLayout, uint32_t newFamilyIndex) const;
    //for swapchain image and internal image works
    void TransferImageLayoutAndQueue(vk::Image image, vk::ImageAspectFlags aspect, vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout, uint32_t oldFamilyIndex, uint32_t newFamilyIndex) const;

    void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);
    void DispatchIndirect(const RawBuffer& buffer, uint32_t offset = 0);

    void BindPipeline(const Pipeline& pipeline);
    void PushConstant(const PipelineLayout& pipelineLayout, ShaderStageFlags pipelineStage, const void* data, uint32_t size, uint32_t offset = 0);

    void BlitImage(const Image& srcImage, const Image& dstImage, const ImageBlitInfo& blitInfo);
    void GenerateMipMaps(const Image& image);
    /*
        void BindVertexBuffer(const Buffer& buffer, uint64_t offset);
        void BindIndexBuffer(const Buffer& buffer, uint64_t offset, vk::IndexType indexType);
     */

    void Draw(uint32_t vertexCount, uint32_t instanceCount);
    void DrawIndirect(const RawBuffer& indirectBuffer, const RawBuffer& countBuffer, uint32_t maxDrawCount);

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount);
    void DrawIndexedIndirect(const RawBuffer& indirectBuffer, const RawBuffer& countBuffer, uint32_t maxDrawCount);

    // using vulkan dynamic rendering
    void BeginRenderPass(RenderPass& renderPass, const glm::vec2& extent, const glm::vec2& offset);
    // using vulkan dynamic rendering
    // a little bit more cpu overhead compared to a render pass
    void BeginRenderPass(const DynamicRenderPassInfo& info);
    void EndRenderPass();

    void SetViewport(const glm::vec2& extent, const glm::vec2& offset = {0, 0}, const glm::vec2& depth = {0, 1});
    void SetScissor(const glm::vec2& extent, const glm::vec2& offset = {0, 0});

    void BeginFrameOptimalFramebufferLayout(const Swapchain& swapchain) const;
    void PresentFrameOptimalFramebufferLayout(const Swapchain& swapchain) const;
    void OptimalSwapchainImageLayout(const Swapchain& swapchain, const uint32_t image_index) const;

    template<typename T>
    void UploadData(RawBuffer& buffer, std::span<const T> data, const uint32_t offset = 0) {
        UploadData(buffer, data.data(), data.size() * sizeof(T), offset);
    }

    template<typename T>
    void UploadData(const Image& image, const ImageSubResource& subResource, std::span<const T> data, const uint32_t offset = 0) {
        UploadData(image, subResource, data.data(), data.size() * sizeof(T), offset);
    }

    void AddDeferredDestroyBuffer(std::unique_ptr<RawBuffer>& buffer) {
        m_deferred_destroy_buffers.emplace_back(std::move(buffer));
    }
private:
    void DestroyDeferredDestroyBuffer() {
        m_deferred_destroy_buffers.clear();
    }
    std::vector<std::unique_ptr<RawBuffer>> m_deferred_destroy_buffers;
    std::unique_ptr<RawBuffer> stagingBuffer = nullptr; // for copy etc.
    vk::raii::CommandBuffer cmdBuffer;
    uint32_t queueFamilyIndex;
    vk::QueueFlags typeFlagBit{};

    friend GraphicsThread;
};

inline void CommandBuffer::Dispatch(const uint32_t x, const uint32_t y, const uint32_t z) {
    typeFlagBit |= vk::QueueFlagBits::eCompute;
    cmdBuffer.dispatch(x, y, z);
}

inline void CommandBuffer::DispatchIndirect(const RawBuffer& buffer, const uint32_t offset) {
    typeFlagBit |= vk::QueueFlagBits::eCompute;
    cmdBuffer.dispatchIndirect(buffer.GetBuffer(), offset);
}

inline void CommandBuffer::BindPipeline(const Pipeline& pipeline) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    typeFlagBit |= vk::QueueFlagBits::eCompute;

    cmdBuffer.bindPipeline(static_cast<vk::PipelineBindPoint>(pipeline.GetBindPoint()), pipeline.GetPipeline());

    if (pipeline.GetUsedDscSets().empty()) return;

    cmdBuffer.bindDescriptorSets(
        static_cast<vk::PipelineBindPoint>(pipeline.GetBindPoint()),
        pipeline.GetPipelineLayout()->GetPipelineLayout(),
        0,
        pipeline.GetUsedDscSets(),
        {});
}

inline void CommandBuffer::PushConstant(const PipelineLayout& pipelineLayout, ShaderStageFlags pipelineStage, const void* data, const uint32_t size, const uint32_t offset) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    typeFlagBit |= vk::QueueFlagBits::eCompute;

    (*cmdBuffer).pushConstants(
        pipelineLayout.GetPipelineLayout(),
        *reinterpret_cast<vk::ShaderStageFlags*>(&pipelineStage),
        offset,
        size,
        data);
}

inline void CommandBuffer::Draw(const uint32_t vertexCount, const uint32_t instanceCount) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.draw(vertexCount, instanceCount, 0, 0);
};

inline void CommandBuffer::DrawIndirect(const RawBuffer& indirectBuffer, const RawBuffer& countBuffer, const uint32_t maxDrawCount) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.drawIndirectCount(indirectBuffer.GetBuffer(), 0, countBuffer.GetBuffer(), 0, maxDrawCount, sizeof(vk::DrawIndirectCommand));
}

inline void CommandBuffer::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.drawIndexed(indexCount, instanceCount, 0, 0, 0);
}

inline void CommandBuffer::DrawIndexedIndirect(const RawBuffer& indirectBuffer, const RawBuffer& countBuffer, const uint32_t maxDrawCount) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.drawIndexedIndirectCount(indirectBuffer.GetBuffer(), 0, countBuffer.GetBuffer(), 0, maxDrawCount, sizeof(vk::DrawIndexedIndirectCommand));
}

inline void CommandBuffer::SetViewport(const glm::vec2& extent, const glm::vec2& offset, const glm::vec2& depth) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.setViewport(0, vk::Viewport{offset.x, offset.y, extent.x, extent.y, depth.x, depth.y});
}

inline void CommandBuffer::SetScissor(const glm::vec2& extent, const glm::vec2& offset) {
    typeFlagBit |= vk::QueueFlagBits::eGraphics;
    cmdBuffer.setScissor(0, vk::Rect2D{
        {static_cast<int>(offset.x), static_cast<int>(offset.y)},
        {static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y)}});
}