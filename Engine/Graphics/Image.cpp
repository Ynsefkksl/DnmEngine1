#include "Graphics/Image.hpp"

#include "Graphics/RawBuffer.hpp"

#include <vma/vk_mem_alloc.h>

#include <iostream>
#include <print>
#include <ranges>

#include "Engine.hpp"
#include "Graphics/GraphicsThread.hpp"

Image::Image(const ImageCreateInfo& createInfo)
    : m_extent(createInfo.extent), m_format(createInfo.format), m_arraySize(createInfo.arraySize), m_mipmapCount(createInfo.mipmapCount), m_imageLayout(createInfo.initLayout)
    , m_usage(createInfo.usage), m_type(createInfo.type), m_memoryUsage(createInfo.memoryUsage) {
    m_vkType = ImageTypeToVkType(m_type);
    m_vkAspect = ImageUsageToVkAspect(m_usage);
    m_vkUsage = ImageUsageToVkUsage(m_usage);

    CreateImage();

    Engine::Get().GetGraphicsThread().SubmitCommands([&] (const CommandBuffer& commandBuffer) -> bool {
        commandBuffer.TransferImageLayoutAndQueue(
            m_image,
            m_vkAspect,
            vk::ImageLayout::eUndefined,
            m_imageLayout, {}, {});

        return true;
    });
}

Image::Image(Image&& other) noexcept
    : m_imageViews(std::move(other.m_imageViews)),
      m_extent(other.m_extent),
      m_image(std::move(other.m_image)),
      m_vmaAllocation(other.m_vmaAllocation),
      m_size(other.m_size),
      m_format(other.m_format),
      m_arraySize(other.m_arraySize),
      m_mipmapCount(other.m_mipmapCount),
      m_queueFamilyIndex(other.m_queueFamilyIndex),
      m_imageLayout(other.m_imageLayout),
      m_vkUsage(other.m_vkUsage),
      m_vkAspect(other.m_vkAspect),
      m_usage(other.m_usage),
      m_vkType(other.m_vkType),
      m_type(other.m_type),
      m_memoryUsage(other.m_memoryUsage) {
    other.m_image = nullptr;
    other.m_vmaAllocation = nullptr;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        m_extent = other.m_extent;
        m_format = other.m_format;
        m_type = other.m_type;
        m_usage = other.m_usage;
        m_arraySize = other.m_arraySize;
        m_imageLayout = other.m_imageLayout;
        m_memoryUsage = other.m_memoryUsage;
        m_vkType = other.m_vkType;
        m_vkUsage = other.m_vkUsage;
        m_vkAspect = other.m_vkAspect;
        m_image = std::move(other.m_image);
        m_size = other.m_size;
        m_mipmapCount = other.m_mipmapCount;
        m_vmaAllocation = other.m_vmaAllocation;

        other.m_image = nullptr;
        other.m_vmaAllocation = nullptr;
    }
    return *this;
}

Image::~Image() {
    m_image.clear();
    vmaFreeMemory(VkContext::Get().GetVmaAllocator(), m_vmaAllocation);
}

void Image::CreateImage() {
    const auto& vkContext = VkContext::Get();

    vk::ImageCreateFlags flags{};
    if (m_type == ImageType::eCube) flags |= vk::ImageCreateFlagBits::eCubeCompatible;
 
    std::vector<uint32_t> usedQueueFamilies {
            vkContext.GetGraphicsQueueFamily(),
            vkContext.GetComputeQueueFamily(), 
            vkContext.GetTransferQueueFamily()
        };

    const VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<uint32_t>(flags),
        .imageType = static_cast<VkImageType>(m_vkType),
        .format = static_cast<VkFormat>(m_format),
        .extent = { m_extent.x, m_extent.y, m_extent.z },
        .mipLevels = m_mipmapCount,
        .arrayLayers = m_arraySize,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = static_cast<VkImageUsageFlags>(m_vkUsage),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,//temp
        .queueFamilyIndexCount = {},//temp
        .pQueueFamilyIndices = {},//temp
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo vmaAllocationCreateInfo{};
    vmaAllocationCreateInfo.usage = VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO;
    vmaAllocationCreateInfo.priority = 1.f;

    if (m_memoryUsage == MemoryType::eDeviceLocal) {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    else if (m_memoryUsage == MemoryType::eCpuWrite) {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    else /*MemoryUsage::eCpuReadWrite*/ {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        vmaAllocationCreateInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }

    VmaAllocationInfo vmaAllocationInfo;

    VkImage image_C;
    auto result = vmaCreateImage(vkContext.GetVmaAllocator(), &imageCreateInfo, &vmaAllocationCreateInfo, &image_C, &m_vmaAllocation, &vmaAllocationInfo);
    m_image = vk::raii::Image(vkContext.GetDevice(), image_C);

    if (result != static_cast<VkResult>(vk::Result::eSuccess))
        LogError("image creating error: {}", vk::to_string(static_cast<vk::Result>(result)));

    m_size = vmaAllocationInfo.size;
}

vk::ImageView Image::CreateGetImageView(const ImageSubResource& subresource) {
    if (m_imageViews.contains(subresource)) {
        return *m_imageViews.at(subresource);
    }

    auto& device = VkContext::Get().GetDevice();

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.setImage(m_image)
                        .setViewType(static_cast<vk::ImageViewType>(subresource.type))
                        .setFormat(m_format)
                        .setComponents(vk::ComponentMapping()
                            .setR(vk::ComponentSwizzle::eIdentity)
                            .setG(vk::ComponentSwizzle::eIdentity)
                            .setB(vk::ComponentSwizzle::eIdentity)
                            .setA(vk::ComponentSwizzle::eIdentity))
                        .setSubresourceRange(vk::ImageSubresourceRange()
                            .setAspectMask(m_vkAspect)
                            .setBaseMipLevel(subresource.baseMipmapLevel)
                            .setLevelCount(subresource.mipmapLevelCount)
                            .setBaseArrayLayer(subresource.baseLayer)
                            .setLayerCount(subresource.layerCount)
                            );

    return *m_imageViews.emplace(subresource, device.createImageView(imageViewCreateInfo)).first->second;
}