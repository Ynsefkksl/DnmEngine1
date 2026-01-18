#pragma once

#include "Graphics/VkContext.hpp"
#include "Graphics/CommandBuffer.hpp"
#include "Graphics/Swapchain.hpp"
#include "Utility/Thread.hpp"

class GraphicsThread {
public:
    GraphicsThread(const vk::Queue& queue, const uint32_t queueFamilyIndex)
        : m_queue(queue), m_command_pool(VK_NULL_HANDLE),
          m_queue_family_index(queueFamilyIndex) {

        auto& device = VkContext::Get().GetDevice();

        vk::CommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.setQueueFamilyIndex(m_queue_family_index);
        commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

        m_command_pool = device.createCommandPool(commandPoolCreateInfo);

        vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo.setCommandPool(*m_command_pool);
        commandBufferAllocateInfo.setCommandBufferCount(1);
        commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);

        m_command_buffer = CommandBuffer::NewUnique(device.allocateCommandBuffers(commandBufferAllocateInfo)[0], m_queue_family_index);

        m_fence = device.createFence({});
    }

    ~GraphicsThread() {
        m_command_buffer.reset();
    }

    void DestroyDeferredDestroyBuffer() const {
        m_command_buffer->DestroyDeferredDestroyBuffer();
    }

    void SubmitCommands(const std::function<bool(CommandBuffer& commandBuffer)>& func, const bool wait_for_fence = true) {
        if (wait_for_fence) WaitForFence();

        m_command_buffer->DestroyDeferredDestroyBuffer();
        m_command_buffer->GetCommandBuffer().reset();

        try {
            m_command_buffer->GetCommandBuffer().begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
            if (!func(*m_command_buffer)) {
                m_command_buffer->GetCommandBuffer().end();
                return;
            }
            m_command_buffer->GetCommandBuffer().end();
        }
        catch (const std::exception& e) {
            LogError("{}", e.what());
        }

        SubmitCommands();
    }

    void PresentFrame(Swapchain& swapchain) const {
        const auto device = *VkContext::Get().GetDevice();

        const uint32_t imageIndex = swapchain.GetImageIndex();
        const vk::SwapchainKHR swapchainVk = swapchain.GetSwapchain();
        const vk::Semaphore semaphore = swapchain.GetSemaphore();
        vk::PresentInfoKHR presentInfo(
            1, // wait Semaphore count
            &semaphore, // wait Semaphores ptr
            1, // swapchain count
            &swapchainVk, // swapchains ptr
            &imageIndex, // image indices ptr
            {} // results ptr
        );

        const vk::Result result = m_queue.presentKHR({presentInfo});

        if (result == vk::Result::eErrorDeviceLost) {
            LogWarning("device lost");
            device.waitIdle();
        }
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            LogWarning("ErrorOutOfDate");
            swapchain.ReCreateSwapchain();
        } else if (result != vk::Result::eSuccess) {
            LogError("failed to present swap chain image!");
        }
    }

    void WaitForFence() {
        try {
            [[maybe_unused]] auto _ = VkContext::Get().GetDevice().waitForFences({m_fence}, vk::True, 1'000'000'000);
        }
        catch (std::exception& e) {
            LogError("{}", e.what());
        }
    }

    // TODO: complete this func
    // CommandBuffer GetCommandBuffer() const {
    //     const auto device = VkContext::Get().GetDevice();
    //
    //     vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    //     commandBufferAllocateInfo.setCommandPool(m_commandPool);
    //     commandBufferAllocateInfo.setCommandBufferCount(1);
    //     commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::eSecondary);
    //
    //     return CommandBuffer(device.allocateCommandBuffers(commandBufferAllocateInfo)[0], m_commandPool, m_queueFamilyIndex);
    // }

    void SubmitCommands() {
        const vk::CommandBufferSubmitInfo cmdSubmitInfo(m_command_buffer->GetCommandBuffer());

        const vk::SubmitInfo2 submitInfo(
            {}, // flags
            0, // wait semphore count
            nullptr, // wait semphore
            1, // cmd buffer info count
            &cmdSubmitInfo, // cmd buffer info
            0, // signal semphore count
            nullptr // signal semphore
        );

        VkContext::Get().GetDevice().resetFences({m_fence});
        m_queue.submit2(submitInfo, m_fence);
    }

    void SubmitCommands(const CommandBuffer& command_buffer) {
        const vk::CommandBufferSubmitInfo cmdSubmitInfo(command_buffer.GetCommandBuffer());

        const vk::SubmitInfo2 submitInfo(
            {}, // flags
            0, // wait semphore count
            nullptr, // wait semphore
            1, // cmd buffer info count
            &cmdSubmitInfo, // cmd buffer info
            0, // signal semphore count
            nullptr // signal semphore
        );

        VkContext::Get().GetDevice().resetFences({m_fence});
        m_queue.submit2(submitInfo, m_fence);
    }

    [[nodiscard]] vk::Queue GetQueue() const { return m_queue; }
    [[nodiscard]] vk::CommandPool GetCommandPool() const { return m_command_pool; }
    [[nodiscard]] CommandBuffer* GetCommandBuffer() const { return m_command_buffer.get(); }
    [[nodiscard]] vk::Fence GetFence() const { return m_fence; }
    [[nodiscard]] uint32_t GetQueueFamilyIndex() const { return m_queue_family_index; }
private:
    const vk::Queue m_queue;
    vk::raii::CommandPool m_command_pool;
    std::unique_ptr<CommandBuffer> m_command_buffer;
    vk::raii::Fence m_fence{nullptr};
    uint32_t m_queue_family_index;
};