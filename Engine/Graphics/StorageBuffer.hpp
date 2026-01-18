#pragma once

#include <Utility/Types.hpp>
#include <Utility/PtrFactory.hpp>
#include <Graphics/RawBuffer.hpp>

#include "GraphicsThread.hpp"

// glsl
// layout(buffer_reference, std430, buffer_reference_align = 8) buffer StorageBuffer {
//     HeaderType header_data;
//     ElementData element_data_array[];
// }

template<typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
class StorageBuffer : public PtrFactory<StorageBuffer<HeaderType, ElementType>> {
public:
    class Handle {
    public:
        Handle() = default;

        Handle(const Handle& other) noexcept = default;
        Handle& operator=(const Handle& other) noexcept = default;
        Handle(Handle&& other) noexcept : value(other.value) {}
        Handle& operator=(Handle&& other) noexcept {
            value = other.value;
            return *this;
        }

        bool operator==(const Handle& other) const {
            return GetValue() == other.GetValue();
        }

        [[nodiscard]] uint32_t GetValue() const {
            return *value;
        }
    private:
        explicit Handle(const uint32_t value) : value(new uint32_t(value)) {}

        void SetValue(const uint32_t newValue) const {
            *value = newValue;
        }

        void Invalidate() {
            value = nullptr;
        }

        [[nodiscard]] bool isNull() const {
            return value == nullptr;
        }

        uint32_t* value{};

        friend class StorageBuffer;
    };

    StorageBuffer(uint64_t reserved_elements, StorageBufferUsageType usage);
    StorageBuffer(StorageBuffer&& other) noexcept = default;
    StorageBuffer& operator=(StorageBuffer&& other) noexcept = default;
    StorageBuffer(const StorageBuffer& other) = delete;
    StorageBuffer& operator=(const StorageBuffer& other) = delete;

    void SetHeader(const HeaderType& data)
        requires NotNull<HeaderType>;

    template<typename T>
    void SetHeader(const T& data, T HeaderType::*member)
        requires NotNull<HeaderType> && NotIntegral<HeaderType>;

    Handle CreateElement(CommandBuffer& command_buffer, const ElementType& element_data)
        requires NotNull<ElementType>;

    std::vector<Handle> CreateElements(CommandBuffer& command_buffer, const std::vector<ElementType>& element_datas)
        requires NotNull<ElementType>;

    void SetElement(const ElementType& data, const Handle& handle)
        requires NotNull<ElementType>;

    template<typename T>
    void SetElement(const T& data, const Handle& handle, T ElementType::*member)
        requires NotNull<ElementType> && NotIntegral<ElementType>;

    void DeleteElement(const Handle& removed_handle)
        requires NotNull<ElementType>;

    void ReserveElement(CommandBuffer& command_buffer, uint64_t count)
        requires NotNull<ElementType>;

    void CopyHandleToHandle(const Handle& src, const Handle& dst) const
        requires NotNull<ElementType>;

    [[nodiscard]] const RawBuffer& GetBuffer() const { return *m_buffer; }

    [[nodiscard]] uint64_t GetByteUsage() const { return GetElementCount() * sizeof(ElementType); }
    [[nodiscard]] uint64_t GetElementCount() const { return m_handles.size(); }
    [[nodiscard]] uint32_t AvailibleSpace() const { return (m_elementCapacity - m_elementCount) * sizeof(ElementType); }
    [[nodiscard]] uint64_t GetBufferDeviceAdress() const;
    [[nodiscard]] uint64_t GetSize() const { return m_buffer->GetSize(); }
    [[nodiscard]] static constexpr uint32_t GetHeaderSize() {
        uint32_t header_size = 0;
        if constexpr (NotNull<HeaderType>) {
            header_size = sizeof(HeaderType);
        }
        return header_size;
    }
private:
    Handle GetEndHandle();
    void RemoveHandle(Handle handle);

    template <class T>
    void CopyToHandle(const T& data, uint32_t index, uint32_t memberOffset) const;

    std::unique_ptr<RawBuffer> m_buffer;
    std::vector<Handle> m_handles;
    ElementType* m_mappedPtr;
    uint64_t m_elementCount{0};
    uint64_t m_elementCapacity{0};
    StorageBufferUsageType m_usageType{};
};

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
StorageBuffer<HeaderType, ElementType>::StorageBuffer(const uint64_t reserved_elements, StorageBufferUsageType usage)
: m_mappedPtr(nullptr), m_elementCapacity(reserved_elements), m_usageType(usage)
{
    m_buffer = RawBuffer::NewUnique(
                    GetHeaderSize() + (m_elementCapacity * sizeof(ElementType)),
                    static_cast<vk::BufferUsageFlags>(static_cast<uint32_t>(usage)),
                    MemoryType::eCpuWrite);

    m_mappedPtr = reinterpret_cast<ElementType*>(m_buffer->MapMemory());
}

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::SetHeader(const HeaderType& data) requires NotNull<HeaderType>
{
    memcpy(reinterpret_cast<char*>(m_mappedPtr), &data, sizeof(HeaderType));
}

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
template <typename T>
void StorageBuffer<HeaderType, ElementType>::SetHeader(const T& data, T HeaderType::* member) requires NotNull<
    HeaderType> && NotIntegral<HeaderType>
{
    memcpy(reinterpret_cast<char*>(m_mappedPtr) + GetMemberOffset(member), &data, sizeof(T));
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
StorageBuffer<HeaderType, ElementType>::Handle StorageBuffer<HeaderType, ElementType>::CreateElement(
    CommandBuffer& command_buffer, const ElementType& element_data) requires NotNull<ElementType>
{
    const uint32_t handleValue = m_elementCount;

    if (m_elementCount >= m_elementCapacity)
        ReserveElement(command_buffer, (m_elementCapacity/2u) + 1u/* for byte usage 0 */);

    CopyToHandle<ElementType>(element_data, GetElementCount(), 0);
    //ReserveElement uses m_elementCount;
    m_elementCount++;

    return std::move(m_handles.emplace_back(Handle(handleValue)));
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
std::vector<typename StorageBuffer<HeaderType, ElementType>::Handle> StorageBuffer<HeaderType, ElementType>::CreateElements(
    CommandBuffer& command_buffer, const std::vector<ElementType>& element_datas) requires NotNull<ElementType>
{
    std::vector<Handle> handles(element_datas.size());

    for (auto i : std::views::iota(m_elementCount, m_elementCount + element_datas.size())) {
        handles.emplace_back(Handle(i));
    }

    if (m_elementCount + element_datas.size() >= m_elementCapacity)
        ReserveElement(command_buffer, (m_elementCapacity/2u) + element_datas.size() /* for byte usage 0 */);

    m_elementCount += element_datas.size();

    auto* writePtr = reinterpret_cast<char*>(m_mappedPtr + GetElementCount()) + GetHeaderSize();
    std::memcpy(writePtr, element_datas.data(), sizeof(ElementType) * element_datas.size());

    return handles;
}

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::SetElement(const ElementType& data,
    const Handle& handle) requires NotNull<ElementType>
{
    if (handle.isNull()) {
        return;
    }

    CopyToHandle<ElementType>(data, handle.GetValue(), 0u);
}

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
template <typename T>
void StorageBuffer<HeaderType, ElementType>::SetElement(const T& data, const Handle& handle, T ElementType::* member)
requires NotNull<ElementType>  && NotIntegral<ElementType>
{
    if (handle.isNull()) {
        return;
    }

    CopyToHandle<T>(data, handle.GetValue(), GetMemberOffset(member));
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::DeleteElement(const Handle& removed_handle) requires NotNull<ElementType>
{
    if (removed_handle.isNull()) {
        return;
    }

    if (GetElementCount() == 1) {
        // element count = m_handles size
        m_handles.clear();
        return;
    }

    const auto endHandle = GetEndHandle();

    if (endHandle == removed_handle) {
        RemoveHandle(removed_handle);
        return;
    }

    CopyHandleToHandle(endHandle, removed_handle);

    endHandle.SetValue(removed_handle.GetValue());

    RemoveHandle(removed_handle);
}

template<typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::ReserveElement(CommandBuffer& command_buffer, const uint64_t count) requires NotNull<ElementType>
{
    m_elementCapacity += count;

    auto newBuffer = RawBuffer::NewUnique(
                        (m_elementCapacity * sizeof(ElementType)) + GetHeaderSize(),
                        static_cast<vk::BufferUsageFlags>(static_cast<uint32_t>(m_usageType)),
                        MemoryType::eCpuWrite);

    command_buffer.CopyBufferToBuffer(*m_buffer, *newBuffer, BufferToBufferCopyInfo {
        .size = m_buffer->GetSize()});

    command_buffer.AddDeferredDestroyBuffer(m_buffer);
    m_buffer = std::move(newBuffer);
    m_mappedPtr = reinterpret_cast<ElementType*>(m_buffer->MapMemory());
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::CopyHandleToHandle(const Handle& src,
                                                    const Handle& dst) const requires NotNull<ElementType>
{
    const ElementType& src_data = *reinterpret_cast<ElementType*>(reinterpret_cast<char*>(m_mappedPtr + src.GetValue()) + GetHeaderSize());
    const auto dstOffset = reinterpret_cast<char*>(m_mappedPtr + dst.GetValue()) + GetHeaderSize();

    memcpy(dstOffset, static_cast<const void*>(&src_data), sizeof(ElementType));
}

template<typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
uint64_t StorageBuffer<HeaderType, ElementType>::GetBufferDeviceAdress() const
{
    return m_buffer->GetBufferAddress();
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
StorageBuffer<HeaderType, ElementType>::Handle StorageBuffer<HeaderType, ElementType>::GetEndHandle()
{
    // Since you create elements in order and, when deleting,
    // you swap the removed item with the last one in both m_spriteBuffer and m_handles,
    // the last element always corresponds to the last handle
    return std::move(m_handles.back());
}

template <typename HeaderType, typename ElementType> requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
void StorageBuffer<HeaderType, ElementType>::RemoveHandle(Handle handle)
{
    if (handle != m_handles.back()) {
        const auto handleIt = m_handles.begin() + handle.GetValue();

        //m_handle last element move to handleIt
        //handleIt now point to endHandle
        *handleIt = std::move(m_handles.back());
        handleIt->SetValue(handle.GetValue());
    }

    m_handles.pop_back();
    handle.Invalidate();
}

template <typename HeaderType, typename ElementType>
requires NotVoid<HeaderType> && NotPointer<HeaderType> && NotIntegral<HeaderType> &&
    NotVoid<ElementType> && NotPointer<ElementType> && NotIntegral<ElementType>
template <typename T>
void StorageBuffer<HeaderType, ElementType>::CopyToHandle(const T& data, const uint32_t index, const uint32_t memberOffset) const
{
    auto* writePtr = reinterpret_cast<char*>(m_mappedPtr + index) + memberOffset + GetHeaderSize();
    std::memcpy(writePtr, &data, sizeof(T));
}
