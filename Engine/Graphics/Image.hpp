#pragma once

#include "Graphics/VkContext.hpp"
#include "Utility/PtrFactory.hpp"

#include <unordered_map>

class ENGINE_API Image : public PtrFactory<Image> {
public:
    explicit Image(const ImageCreateInfo& createInfo);
    Image(Image &&) noexcept;
    Image &operator=(Image &&) noexcept;
    ~Image();

    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;
    
    [[nodiscard]] vk::ImageView CreateGetImageView(const ImageSubResource& subresource);

    [[nodiscard]] ColorAttachmentInfo GetColorAttachmentInfo(
        const ImageSubResource& subresource,
        const glm::vec4& clearValue = {0,0,0,0},
        AttachmentLoadOp loapOp = AttachmentLoadOp::eClear,
        AttachmentStoreOp storeOp = AttachmentStoreOp::eStore);

    [[nodiscard]] DepthAttachmentInfo GetDepthAttachmentInfo(
        const ImageSubResource& subresource,
        DepthStencilClear clearValue = {1.f, 0},
        AttachmentLoadOp loapOp = AttachmentLoadOp::eClear,
        AttachmentStoreOp storeOp = AttachmentStoreOp::eStore);

    [[nodiscard]] FORCE_INLINE vk::Image GetImage() const { return m_image; }
    [[nodiscard]] FORCE_INLINE VmaAllocation GetVmaAllocation() const { return m_vmaAllocation; }
    [[nodiscard]] FORCE_INLINE vk::ImageLayout GetImageLayout() const { return m_imageLayout; }
    [[nodiscard]] FORCE_INLINE ImageFormat GetFormat() const { return m_format; }
    [[nodiscard]] FORCE_INLINE ImageType GetType() const { return m_type; }
    [[nodiscard]] FORCE_INLINE ImageUsageFlags GetUsage() const { return m_usage; }
    [[nodiscard]] FORCE_INLINE glm::uvec3 GetExtent() const { return m_extent; }
    [[nodiscard]] FORCE_INLINE MemoryType GetMemoryUsage() const { return m_memoryUsage; }
    [[nodiscard]] FORCE_INLINE uint32_t GetArraySize() const { return m_arraySize; }
    [[nodiscard]] FORCE_INLINE uint32_t GetMipmapCount() const { return m_mipmapCount; }
    [[nodiscard]] FORCE_INLINE uint64_t GetSize() const { return m_size; }
    [[nodiscard]] FORCE_INLINE bool IsDepthMap() const { return m_usage.Has(ImageUsageBit::eDepthStencilAttachment); }
    [[nodiscard]] FORCE_INLINE bool IsArrayed() const { return m_arraySize != 1; }
    [[nodiscard]] FORCE_INLINE uint32_t GetQueueFamily() const { return m_queueFamilyIndex; }
    [[nodiscard]] FORCE_INLINE vk::ImageAspectFlags GetAspect() const { return m_vkAspect; }

    //declarations below
    [[nodiscard]] static vk::ImageType ImageTypeToVkType(ImageType type);
    [[nodiscard]] static vk::ImageAspectFlags ImageUsageToVkAspect(ImageUsageFlags usage);
    [[nodiscard]] static vk::ImageUsageFlags ImageUsageToVkUsage(ImageUsageFlags usage);
private:
    void CreateImage();

    std::unordered_map<ImageSubResource, vk::raii::ImageView> m_imageViews;

    glm::uvec3 m_extent;
    vk::raii::Image m_image{nullptr};
    VmaAllocation m_vmaAllocation{};
    uint64_t m_size{};
    ImageFormat m_format;
    uint32_t m_arraySize;
    uint32_t m_mipmapCount;
    uint32_t m_queueFamilyIndex{};
    vk::ImageLayout m_imageLayout = vk::ImageLayout::eUndefined;
    vk::ImageUsageFlags m_vkUsage;
    vk::ImageAspectFlags m_vkAspect;
    ImageUsageFlags m_usage;
    vk::ImageType m_vkType;
    ImageType m_type;
    MemoryType m_memoryUsage;
    //vk::format equal ImageFormat

    friend class CommandBuffer;
};

inline vk::ImageType Image::ImageTypeToVkType(const ImageType type) {
    switch (type) {
        case ImageType::e1D: return vk::ImageType::e1D;
        case ImageType::e2D: return vk::ImageType::e2D;
        case ImageType::e3D: return vk::ImageType::e3D;
        case ImageType::eCube: return vk::ImageType::e2D;
        default: return vk::ImageType::e2D;
    }
}

inline vk::ImageAspectFlags Image::ImageUsageToVkAspect(const ImageUsageFlags usage) {
    if (usage.Has(ImageUsageBit::eDepthAttachment))
        return vk::ImageAspectFlagBits::eDepth;

    if (usage.Has(ImageUsageBit::eStencilAttachment))
        return vk::ImageAspectFlagBits::eStencil;

    if (usage.Has(ImageUsageBit::eDepthStencilAttachment))
        return (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);

    if (usage.Has(ImageUsageBit::eColorAttachment) || usage.Has(ImageUsageBit::eSampled) || usage.Has(ImageUsageBit::eStorage))
        return vk::ImageAspectFlagBits::eColor;

    return vk::ImageAspectFlagBits::eNone;
}

inline vk::ImageUsageFlags Image::ImageUsageToVkUsage(const ImageUsageFlags usage) {
    return vk::ImageUsageFlags(static_cast<uint8_t>(usage));
}

inline ColorAttachmentInfo Image::GetColorAttachmentInfo(
    const ImageSubResource& subresource,
    const glm::vec4& clearValue,
    const AttachmentLoadOp loapOp,
    const AttachmentStoreOp storeOp) {

    return {.clearValue = clearValue, .imageView = CreateGetImageView(subresource), .imageLayout = m_imageLayout, .loapOp = loapOp, .storeOp = storeOp};
}

inline DepthAttachmentInfo Image::GetDepthAttachmentInfo(
    const ImageSubResource& subresource,
    const DepthStencilClear clearValue,
    const AttachmentLoadOp loapOp,
    const AttachmentStoreOp storeOp) {

    return {.clearValue = clearValue, .imageView = CreateGetImageView(subresource),  .imageLayout = m_imageLayout, .loapOp = loapOp, .storeOp = storeOp};
}
