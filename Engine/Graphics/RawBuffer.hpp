#pragma once

#include <ranges>

#include "Engine.hpp"
#include "Graphics/VkContext.hpp"
#include "Utility/PtrFactory.hpp"
#include "Utility/Types.hpp"

class ENGINE_API RawBuffer : public PtrFactory<RawBuffer> {
public:
    RawBuffer(uint64_t size, vk::BufferUsageFlags usage, MemoryType memoryUsage);

    RawBuffer(RawBuffer&&) noexcept;
    RawBuffer(RawBuffer&) = delete;
    RawBuffer& operator=(RawBuffer&&) noexcept;
    RawBuffer& operator=(RawBuffer&) = delete;
    ~RawBuffer();

    [[nodiscard]] FORCE_INLINE vk::Buffer GetBuffer() const { return *m_buffer; }
    [[nodiscard]] FORCE_INLINE vk::BufferUsageFlags GetUsageType() const { return m_bufferUsage; }
    [[nodiscard]] FORCE_INLINE uint64_t GetSize() const { return m_size; }
    [[nodiscard]] FORCE_INLINE VmaAllocation GetVmaAllocation() const { return m_vmaAllocation; }
    [[nodiscard]] FORCE_INLINE uint32_t GetQueueFamily() const { return  m_queueFamilyIndex; }
    [[nodiscard]] FORCE_INLINE bool IsCpuVisible() const { return static_cast<bool>(m_memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible); }

    [[nodiscard]] char* MapMemory();
    template<typename T = void> [[nodiscard]] FORCE_INLINE T* MapMemory() { return reinterpret_cast<T*>(MapMemory()); }
    void UnmapMemory();

    void SetDebugName(std::string_view debugName) const;
    [[nodiscard]] uint64_t GetBufferAddress() const;
private:
    void CreateBuffer(uint64_t size);

    vk::raii::Buffer m_buffer{VK_NULL_HANDLE};
    VmaAllocation m_vmaAllocation;

    MemoryType m_memoryUsage;
    vk::BufferUsageFlags m_bufferUsage;

    uint64_t m_size;
    uint32_t m_queueFamilyIndex;
    char* m_mappedPtr = nullptr;
    vk::MemoryPropertyFlags m_memoryProperties;

    friend class CommandBuffer;
};

class ENGINE_API UniformBuffer : public PtrFactory<UniformBuffer> {
public:
    explicit UniformBuffer(const uint64_t size)
        : m_buffer(RawBuffer::NewUnique(size,
                            vk::BufferUsageFlagBits::eUniformBuffer
                            | vk::BufferUsageFlagBits::eTransferSrc
                            | vk::BufferUsageFlagBits::eTransferDst,
                            MemoryType::eCpuWrite)) {}

    [[nodiscard]] const RawBuffer& GetBuffer() const { return *m_buffer; }
    [[nodiscard]] void* GetMappedPtr() const { return m_buffer->MapMemory(); }
private:
    const std::unique_ptr<RawBuffer> m_buffer;
};