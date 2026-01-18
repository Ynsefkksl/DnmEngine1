#include "Graphics/Swapchain.hpp"

#include "Engine.hpp"
#include "Graphics/GraphicsThread.hpp"
#include "Utility/Counter.hpp"

static vk::Extent2D ChooseSwapExtent() {
    const auto surface = *VkContext::Get().GetSurface();
    const auto physicalDevice = *VkContext::Get().GetPhysicalDevice();

    if (const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface); capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    else {
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(GetWindow().GetExtent().x),
            static_cast<uint32_t>(GetWindow().GetExtent().y)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

static bool IsItSupportedSurfaceFormat(const vk::Format format, const vk::ColorSpaceKHR colorSpace) {
    const auto surface = *VkContext::Get().GetSurface();
    const auto physicalDevice = VkContext::Get().GetPhysicalDevice();

    return std::ranges::any_of(
            physicalDevice.getSurfaceFormatsKHR(surface),
            [format, colorSpace](auto availableFormat) { return availableFormat.format == format && availableFormat.colorSpace == colorSpace; }
        );
}

static bool IsItSupportedSurfacePresentMode(const vk::PresentModeKHR presentMode) {
    const auto surface = *VkContext::Get().GetSurface();
    const auto physicalDevice = VkContext::Get().GetPhysicalDevice();

    return std::ranges::any_of(
            physicalDevice.getSurfacePresentModesKHR(surface),
            [presentMode](auto availablePresentMode) { return availablePresentMode == presentMode; }
        );
}

Swapchain::Swapchain(const vk::ColorSpaceKHR colorSpace, const vk::Format format, const vk::PresentModeKHR presentMode)
    : m_format(format), m_colorSpace(colorSpace), m_presentMode(presentMode) {
    const auto device = *VkContext::Get().GetDevice();
    
    CreateSwapchain(colorSpace, format, presentMode);

    //create sync object
    constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo;

    m_semaphore = device.createSemaphore(semaphoreCreateInfo);
}

void Swapchain::CreateSwapchain(const vk::ColorSpaceKHR colorSpace, const vk::Format format, const vk::PresentModeKHR presentMode) {
    const auto& vkContext = VkContext::Get();
    const auto physicalDevice = vkContext.GetPhysicalDevice();
    const auto device = *vkContext.GetDevice();
    const auto& surface = VkContext::Get().GetSurface();

    device.waitIdle();

    const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    m_extent = ChooseSwapExtent();
    if (!IsItSupportedSurfaceFormat(format, colorSpace)) {
        const auto surfaceFormat = physicalDevice.getSurfaceFormatsKHR(surface)[0];
        m_format = surfaceFormat.format;
        m_colorSpace = surfaceFormat.colorSpace;
    }

    if (!IsItSupportedSurfacePresentMode(presentMode))
        m_presentMode = vk::PresentModeKHR::eFifo;

    uint32_t imageCount = 3;
    
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.setSurface(surface)
            .setMinImageCount(imageCount)
            .setImageFormat(m_format)
            .setImageColorSpace(m_colorSpace)
            .setImageExtent(m_extent)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setPreTransform(capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode(m_presentMode)
            .setClipped(vk::True)
            .setOldSwapchain(m_swapchain)
            .setQueueFamilyIndexCount(0)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPQueueFamilyIndices(nullptr);

    m_swapchain = device.createSwapchainKHR(createInfo);

    m_images = device.getSwapchainImagesKHR(m_swapchain);

    //create image view and change image layout to present src
    m_imageViews.resize(m_images.size());

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.setImage(VK_NULL_HANDLE)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(m_format)
            .setComponents(vk::ComponentMapping()
                        .setR(vk::ComponentSwizzle::eIdentity)
                        .setG(vk::ComponentSwizzle::eIdentity)
                        .setB(vk::ComponentSwizzle::eIdentity)
                        .setA(vk::ComponentSwizzle::eIdentity))
                        .setSubresourceRange(vk::ImageSubresourceRange()
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseMipLevel(0)
                        .setLevelCount(1)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1));

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (const CommandBuffer& commandBuffer) -> bool {
        for (const auto i : Counter(m_imageViews.size())) {
            imageViewCreateInfo.setImage(m_images[i]);

            m_imageViews[i] = device.createImageView(imageViewCreateInfo);

            commandBuffer.TransferImageLayoutAndQueue(
                m_images[i],
                vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED);
        }
        return true;
    });
}

Swapchain::~Swapchain() {
    const auto device = *VkContext::Get().GetDevice();
    device.waitIdle();

    device.destroy(m_semaphore);

    for (const auto& imageView : m_imageViews)
        device.destroy(imageView);
    
    device.destroySwapchainKHR(m_swapchain);
}

void Swapchain::AcquireNextImage() {
    const auto device = *VkContext::Get().GetDevice();

    const vk::AcquireNextImageInfoKHR acquireNextInfo(
        m_swapchain,
        1'000'000'000,
        m_semaphore,
        nullptr,
        1
    );

    const auto result = device.acquireNextImage2KHR(acquireNextInfo);

    if (result.result == vk::Result::eErrorDeviceLost) {
        LogWarning("device lost");
        device.waitIdle();
    }
    if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR) {
        LogWarning("ErrorOutOfDate");
    } else if (result.result != vk::Result::eSuccess) {
        LogError("failed to present swap chain image!");
    }

    m_imageIndex = result.value;
}
