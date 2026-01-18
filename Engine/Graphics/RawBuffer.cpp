#include "Graphics/RawBuffer.hpp"

#include <vma/vk_mem_alloc.h>
#include "Graphics/Image.hpp"

#include <iostream>

#include "Engine.hpp"

RawBuffer::RawBuffer(const uint64_t size, const vk::BufferUsageFlags usage, const MemoryType memoryUsage)
: m_vmaAllocation(nullptr), m_memoryUsage(memoryUsage), m_bufferUsage(usage) {
    CreateBuffer(size);
}

RawBuffer::RawBuffer(RawBuffer&& other) noexcept
    : m_buffer(std::move(other.m_buffer)),
      m_vmaAllocation(other.m_vmaAllocation),
      m_memoryUsage(other.m_memoryUsage),
      m_bufferUsage(other.m_bufferUsage),
      m_size(other.m_size),
      m_queueFamilyIndex(other.m_queueFamilyIndex) {
    other.m_vmaAllocation = nullptr;
}

RawBuffer& RawBuffer::operator=(RawBuffer&& other) noexcept {
    if (this != &other) {
        m_buffer = std::move(other.m_buffer);
        m_bufferUsage = other.m_bufferUsage;
        m_memoryUsage = other.m_memoryUsage;
        m_vmaAllocation = other.m_vmaAllocation;
        m_mappedPtr = other.m_mappedPtr;

        other.m_vmaAllocation = nullptr;
    }
    return *this;
}

RawBuffer::~RawBuffer() {
    if (m_mappedPtr)
        UnmapMemory();

    m_buffer.clear();
    vmaFreeMemory(VkContext::Get().GetVmaAllocator(), m_vmaAllocation);
}

char* RawBuffer::MapMemory() {
    if (m_memoryUsage == MemoryType::eDeviceLocal || m_mappedPtr != nullptr)
        return m_mappedPtr; // if mem type device local, mappedPtr = nullptr

    void* data;
    vmaMapMemory(VkContext::Get().GetVmaAllocator(), m_vmaAllocation, &data);
    m_mappedPtr = static_cast<char*>(data);
    return m_mappedPtr;
}

void RawBuffer::UnmapMemory() {
    m_mappedPtr = nullptr;
    vmaUnmapMemory(VkContext::Get().GetVmaAllocator(), m_vmaAllocation);
}

void RawBuffer::SetDebugName(const std::string_view debugName) const {
    if constexpr (debug == false) return;

    VkBuffer buffer = *m_buffer;

    vk::DebugUtilsObjectNameInfoEXT info;
    info.objectHandle = reinterpret_cast<uint64_t>(buffer);
    info.objectType = vk::ObjectType::eBuffer;
    info.pObjectName = debugName.data();
    VkContext::Get().GetDevice().setDebugUtilsObjectNameEXT(info);
}

uint64_t RawBuffer::GetBufferAddress() const {
    if (m_bufferUsage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
        return VkContext::Get().GetDevice().getBufferAddress({m_buffer});
    return 0;
}

void RawBuffer::CreateBuffer(const uint64_t size) {
    const auto& vkContext = VkContext::Get();

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = static_cast<VkBufferUsageFlags>(m_bufferUsage);
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//temp
    bufferCreateInfo.queueFamilyIndexCount = {};//temp
    bufferCreateInfo.pQueueFamilyIndices = {};//temp

    VmaAllocationCreateInfo vmaAllocationCreateInfo{};
    vmaAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocationCreateInfo.priority = 1.f;

    if (m_memoryUsage == MemoryType::eDeviceLocal) {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    else if (m_memoryUsage == MemoryType::eCpuWrite) {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    else /*MemoryUsage::eCpuReadWrite*/ {
        vmaAllocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        vmaAllocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    VmaAllocationInfo vmaAllocationInfo;

    VkBuffer buffer_C;
    auto result = vmaCreateBuffer(vkContext.GetVmaAllocator(), &bufferCreateInfo, &vmaAllocationCreateInfo, &buffer_C, &m_vmaAllocation, &vmaAllocationInfo);
    m_buffer = vk::raii::Buffer(VkContext::Get().GetDevice(), buffer_C);

    if (result != static_cast<VkResult>(vk::Result::eSuccess))
        LogError("buffer creating error: {}", vk::to_string(static_cast<vk::Result>(result)));

    m_size = vmaAllocationInfo.size;
    m_memoryProperties = VkContext::Get().GetMemoryProperties().memoryTypes[vmaAllocationInfo.memoryType].propertyFlags;
}
