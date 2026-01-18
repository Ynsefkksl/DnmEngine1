#pragma once

#include "Graphics/VkContext.hpp"
#include "Utility/PtrFactory.hpp"

class Swapchain : public PtrFactory<Swapchain> {
public:
    Swapchain(vk::ColorSpaceKHR colorSpace, vk::Format format, vk::PresentModeKHR presentMode);
    ~Swapchain();

    [[nodiscard]] ColorAttachmentInfo GetColorAttachmentInfo(
        const glm::vec4& clearValue,
        AttachmentLoadOp loapOp,
        AttachmentStoreOp storeOp) const;

    [[nodiscard]] ColorAttachmentInfo GetColorAttachmentInfo(
        uint32_t index,
        const glm::vec4& clearValue,
        AttachmentLoadOp loapOp,
        AttachmentStoreOp storeOp) const;

    void AcquireNextImage();

    void ReCreateSwapchain() { CreateSwapchain(m_colorSpace, m_format, m_presentMode); };
    void CreateSwapchain(vk::ColorSpaceKHR colorSpace, vk::Format format, vk::PresentModeKHR presentMode);

    [[nodiscard]] FORCE_INLINE vk::SwapchainKHR GetSwapchain() const { return m_swapchain; }
    [[nodiscard]] FORCE_INLINE vk::Format GetFormat() const { return m_format; }
    [[nodiscard]] FORCE_INLINE vk::ColorSpaceKHR GetColorSpace() const { return m_colorSpace; }
    [[nodiscard]] FORCE_INLINE vk::PresentModeKHR GetPresentMode() const { return m_presentMode; }
    [[nodiscard]] FORCE_INLINE vk::Extent2D GetExtent() const { return m_extent; }
    [[nodiscard]] FORCE_INLINE const std::vector<vk::Image>& GetImages() const { return m_images; }
    [[nodiscard]] FORCE_INLINE const std::vector<vk::ImageView>& GetImageViews() const { return m_imageViews; }
    [[nodiscard]] FORCE_INLINE uint32_t GetImageIndex() const { return m_imageIndex; }
    [[nodiscard]] FORCE_INLINE vk::Semaphore GetSemaphore() const { return m_semaphore; }

private:
    vk::SwapchainKHR m_swapchain = VK_NULL_HANDLE;

    vk::Format m_format;
    vk::ColorSpaceKHR m_colorSpace;
    vk::PresentModeKHR m_presentMode;
    vk::Extent2D m_extent{0, 0};

    std::vector<vk::Image> m_images{};
    std::vector<vk::ImageView> m_imageViews{};
    
    vk::Semaphore m_semaphore;
    vk::Fence m_fence;

    uint32_t m_imageIndex = 0;
};

inline ColorAttachmentInfo Swapchain::GetColorAttachmentInfo(
        const glm::vec4& clearValue,
        const AttachmentLoadOp loapOp,
        const AttachmentStoreOp storeOp) const {

    return {.clearValue = clearValue, .imageView = m_imageViews[m_imageIndex], .imageLayout = vk::ImageLayout::eAttachmentOptimal, .loapOp = loapOp, .storeOp = storeOp};
}

inline ColorAttachmentInfo Swapchain::GetColorAttachmentInfo(
        const uint32_t index,
        const glm::vec4& clearValue,
        const AttachmentLoadOp loapOp,
        const AttachmentStoreOp storeOp) const {

    return {.clearValue = clearValue, .imageView = m_imageViews[index], .imageLayout = vk::ImageLayout::eAttachmentOptimal, .loapOp = loapOp, .storeOp = storeOp};
}