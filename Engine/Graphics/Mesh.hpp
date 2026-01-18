#pragma once

#include "Graphics/StorageBuffer.hpp"
#include "Utility/UnorderedArray.hpp"

// u16 is used only to store f16 values
struct DefualtVertex {
    glm::u16vec4 position;//f16vec3 support is bad
    glm::u16vec4 tangent;
    glm::u16vec4 normal;//f16vec3 support is bad
    glm::u16vec2 uv;
};

struct DefaultInstanceData {
    glm::mat4 model_mtx;
};

struct MeshManagerCreateInfo {
    // vertex buffer vertex capacity
    uint32_t reservedVertices = 500'000;
    // index buffer index capacity
    uint32_t reservedIndices = 500'000;
    // indirect buffer capacity
    uint32_t reservedSubmeshes = 10'000;
};

template <typename VertexData = NullData, typename InstanceData = NullData>
class MeshManager;

class Submesh {
public:
    //for glsl alignment
    struct alignas(8) MaterialHandleAligned { uint32_t material_handle; MaterialHandleAligned(const uint32_t handle) : material_handle(handle) {} };
    struct alignas(8) InstanceBufferPointer { uint64_t ptr; InstanceBufferPointer(const uint64_t ptr) : ptr(ptr) {} };

    using InstanceHandle = StorageBuffer<MaterialHandleAligned, InstanceBufferPointer>::Handle;
    using IndirectBufferHandle = StorageBuffer<NullData, vk::DrawIndexedIndirectCommand>::Handle;

    Submesh(const uint32_t material_handle, const uint32_t reserved_instance_count, IndirectBufferHandle& indirect_handle)
    : m_data_buffer(reserved_instance_count, StorageBufferUsageType::eOnlyStorageBuffer),
        m_indirect_handle(std::move(indirect_handle)) {
        SetMaterial(material_handle);
    }

    Submesh(Submesh&& other) noexcept
        : m_data_buffer(std::move(other.m_data_buffer)),
            m_indirect_handle(std::move(other.m_indirect_handle)),
            m_material_handle(other.m_material_handle) {}

    Submesh& operator=(Submesh&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        m_data_buffer = std::move(other.m_data_buffer);
        m_indirect_handle = std::move(other.m_indirect_handle);
        m_material_handle = other.m_material_handle;
        return *this;
    }

    Submesh(const Submesh& other) = delete;
    Submesh& operator=(Submesh& other) = delete;

    ~Submesh() = default;

    void SetMaterial(const uint32_t material_handle)
    {
        m_material_handle = material_handle;
        m_data_buffer.SetHeader(MaterialHandleAligned{m_material_handle});
    }

    [[nodiscard]] InstanceHandle CreateInstance(CommandBuffer& command_buffer, const InstanceBufferPointer& data)
    {
        return m_data_buffer.CreateElement(command_buffer, data);
    }

    [[nodiscard]] std::vector<InstanceHandle> CreateInstances(CommandBuffer& command_buffer, const std::vector<InstanceBufferPointer>& data)
    {
        return m_data_buffer.CreateElements(command_buffer, data);
    }

    void SetInstancePtr(const uint64_t& data, const InstanceHandle& handle)
    {
        m_data_buffer.SetElement(data, handle);
    }

    void RemoveInstance(const InstanceHandle& handle)
    {
        m_data_buffer.DeleteElement(handle);
    }

    void ReserveInstance(CommandBuffer& command_buffer, const uint32_t reserve_count)
    {
        m_data_buffer.ReserveElement(command_buffer, reserve_count);
    }

    [[nodiscard]] uint64_t GetDataBufferDeviceAddress() const
    {
        return m_data_buffer.GetBufferDeviceAdress();
    }

    [[nodiscard]] uint32_t GetUsageCount() const { return m_data_buffer.GetElementCount(); }
    [[nodiscard]] IndirectBufferHandle GetIndirectBufferHandle() const { return m_indirect_handle; }
private:
    // layout(buffer_reference, std430, buffer_reference_align = 8) buffer readonly restrict SubmeshDataBuffer {
    //      uint material_handle; vec3 padding;
    //      uvec2 instance_buffer_ptr_array[];
    // };
    StorageBuffer<MaterialHandleAligned, InstanceBufferPointer> m_data_buffer;
    IndirectBufferHandle m_indirect_handle;
    uint32_t m_material_handle;
};

template <typename VertexData, typename InstanceData>
class Mesh : public PtrFactory<Mesh<VertexData, InstanceData>> {
public:
    using ManagerType = MeshManager<VertexData, InstanceData>;
    using SubmeshHandle = UnorderedArray<Submesh*>::Handle;

    explicit Mesh(ManagerType& manager)
        : m_data_buffer(sizeof(InstanceData), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress, MemoryType::eCpuWrite),
            m_manager(&manager) {}

    void SetInstanceData(const InstanceData& data) {
        auto* mapped_ptr = reinterpret_cast<InstanceData*>(m_data_buffer.MapMemory());
        *mapped_ptr = data;
    }

    template <typename T>
    void SetInstanceMember(const T& data, T InstanceData::*member) {
        T* mapped_ptr = static_cast<T*>(m_data_buffer.MapMemory() + GetMemberOffset(member));
        *mapped_ptr = data;
    }

    SubmeshHandle UseSubmesh(CommandBuffer& command_buffer, Submesh* submesh) {
        m_submesh_handles.emplace_back(submesh->CreateInstance(command_buffer, m_data_buffer.GetBufferAddress()));
        return m_submeshes.AddElement(submesh);
    }

    void RemoveSubmesh(SubmeshHandle& handle) {
        m_submeshes.GetElement(handle)->RemoveInstance(m_submesh_handles[handle.GetValue()]);
        m_submeshes.DeleteElement(handle);
    }

private:
    RawBuffer m_data_buffer;
    UnorderedArray<Submesh*> m_submeshes;
    std::vector<Submesh::InstanceHandle> m_submesh_handles;
    ManagerType* m_manager;
};

template <typename VertexData, typename InstanceData>
class MeshManager {
public:
    using MeshType = Mesh<VertexData, InstanceData>;
    using MeshPtr = std::shared_ptr<Mesh<VertexData, InstanceData>>;
    using Vertices = std::span<const VertexData>;
    using Indices = std::span<const uint32_t>;
    using SubmeshHandle = StorageBuffer<NullData, vk::DrawIndexedIndirectCommand>::Handle;
    //using SubmeshDataHandle = StorageBuffer<uint64_t, InstanceData>::Handle;

    struct SubmeshBufferPtr { uint64_t ptr; };

    explicit MeshManager(const MeshManagerCreateInfo& create_info = MeshManagerCreateInfo());
    MeshManager(const MeshManager&) = delete;
    MeshManager& operator=(const MeshManager&) = delete;
    MeshManager(MeshManager&& other) noexcept = delete;
    MeshManager& operator=(MeshManager&& other) noexcept = delete;

    void Draw(CommandBuffer& command_buffer, const PipelineLayout& pipeline_layout) const {
        const uint64_t bda = m_submesh_buffer.GetBufferDeviceAdress();
        command_buffer.PushConstant(pipeline_layout, ShaderStageBit::eVertex, &bda, sizeof(bda));

        command_buffer.GetCommandBuffer().bindVertexBuffers(0, {m_vertex_buffer->GetBuffer()}, {0});
        command_buffer.GetCommandBuffer().bindIndexBuffer(m_index_buffer->GetBuffer(), 0, vk::IndexType::eUint32);
        command_buffer.DrawIndexedIndirect(m_indirect_buffer.GetBuffer(), *m_count_buffer, GetSubmeshCount());
    }

    void Draw(CommandBuffer& command_buffer) const {
        command_buffer.GetCommandBuffer().bindVertexBuffers(0, {m_vertex_buffer->GetBuffer()}, {0});
        command_buffer.GetCommandBuffer().bindIndexBuffer(m_index_buffer->GetBuffer(), 0, vk::IndexType::eUint32);
        command_buffer.DrawIndexedIndirect(m_indirect_buffer.GetBuffer(), *m_count_buffer, GetSubmeshCount());
    }

    Submesh* CreateSubmesh(CommandBuffer& command_buffer, const uint32_t material_handle, const RawBuffer& vertices, const RawBuffer& indices) {
        auto indirect_command = UploadDataVertexIndexBuffer(command_buffer, vertices, indices);
        indirect_command.instanceCount = 1;

        auto indirect_handle = m_indirect_buffer.CreateElement(command_buffer, indirect_command);

        auto* outSubmesh = m_submeshes.emplace_back(std::make_unique<Submesh>(material_handle, 32, indirect_handle)).get();

        m_submesh_buffer.CreateElement(command_buffer, SubmeshBufferPtr{outSubmesh->GetDataBufferDeviceAddress()});
        return outSubmesh;
    }

    std::shared_ptr<MeshType> CreateMesh(CommandBuffer& command_buffer, const std::vector<Submesh*>& submeshes) {
        std::shared_ptr<MeshType> outMesh = std::make_shared<MeshType>(*this);

        for (auto& submesh : submeshes) {
            outMesh->UseSubmesh(command_buffer, submesh);
        }

        m_draw_count += submeshes.size();

        auto* mappedPtr = m_count_buffer->MapMemory<uint32_t>();
        *mappedPtr = m_draw_count;
        m_count_buffer->UnmapMemory();

        return outMesh;
    }

    [[nodiscard]] uint32_t GetSubmeshCount() const { return m_submeshes.size(); }
    [[nodiscard]] uint32_t GetVertexCount() const { return m_vertex_count; }
    [[nodiscard]] uint32_t GetIndexCount() const { return m_index_count; }

    [[nodiscard]] uint32_t GetVertexBufferByteUsage() const { return m_vertex_count * sizeof(VertexData); }
    [[nodiscard]] uint32_t GetIndexBufferByteUsage() const { return m_index_count * sizeof(uint32_t); }

    [[nodiscard]] const auto& GetSubmeshBuffer() const { return m_submesh_buffer; }
    [[nodiscard]] auto* GetVertexBuffer() const { return m_vertex_buffer.get(); }
    [[nodiscard]] auto* GetIndexBuffer() const { return m_index_buffer.get(); }

private:
    vk::DrawIndexedIndirectCommand UploadDataVertexIndexBuffer(CommandBuffer& command_buffer,
                                                               const RawBuffer& vertices,
                                                               const RawBuffer& indices);

    void Reserve(CommandBuffer& command_buffer, uint32_t reserve_vertices, uint32_t reserve_indices, uint32_t reserve_submesh);

    std::vector<std::unique_ptr<Submesh>> m_submeshes;
    StorageBuffer<NullData, SubmeshBufferPtr> m_submesh_buffer; // contains submesh ptrs
    StorageBuffer<NullData, vk::DrawIndexedIndirectCommand> m_indirect_buffer;
    std::vector<uint32_t> m_submesh_usage_count{};

    std::unique_ptr<RawBuffer> m_count_buffer{};
    std::unique_ptr<RawBuffer> m_vertex_buffer{};
    std::unique_ptr<RawBuffer> m_index_buffer{};
    uint32_t m_vertex_count{};
    uint32_t m_index_count{};
    uint32_t m_draw_count{};
};

template <typename VertexData, typename InstanceData>
MeshManager<VertexData, InstanceData>::MeshManager(const MeshManagerCreateInfo& create_info)
    : m_submesh_buffer(create_info.reservedSubmeshes, StorageBufferUsageType::eOnlyStorageBuffer),
    m_indirect_buffer(create_info.reservedSubmeshes, StorageBufferUsageType::eIndirect) {

    m_submesh_buffer.GetBuffer().SetDebugName("SubmeshBuffer");

    m_vertex_buffer = RawBuffer::NewUnique(
        create_info.reservedVertices * sizeof(VertexData),
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        MemoryType::eDeviceLocal);

    m_index_buffer = RawBuffer::NewUnique(
        create_info.reservedIndices * sizeof(uint32_t),
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        MemoryType::eDeviceLocal);

    m_count_buffer = RawBuffer::NewUnique(
        sizeof(uint32_t),
        vk::BufferUsageFlagBits::eIndirectBuffer,
        MemoryType::eCpuWrite);
}

template <typename VertexData, typename InstanceData>
vk::DrawIndexedIndirectCommand MeshManager<VertexData, InstanceData>::UploadDataVertexIndexBuffer(
    CommandBuffer& command_buffer,
    const RawBuffer& vertices, const RawBuffer& indices)
{
    if (m_vertex_buffer->GetSize() < vertices.GetSize() + GetVertexBufferByteUsage()
        || m_index_buffer->GetSize() < indices.GetSize() + GetIndexBufferByteUsage()) {
        Reserve(command_buffer, 2 * vertices.GetSize()/sizeof(VertexData),2 * indices.GetSize()/sizeof(uint32_t), 0);
        PipelineBarrierInfo barrier = {
            .srcStageFlags = PipelineStageBit::eTransfer,
            .dstStageFlags = PipelineStageBit::eTransfer,
            .srcAccessFlags = AccessBit::eTransferWrite,
            .dstAccessFlags = AccessBit::eTransferRead,
        };
        command_buffer.PipelineBarrier({&barrier, 1});
    }


    const auto out_indirect_command = vk::DrawIndexedIndirectCommand(
        static_cast<uint32_t>(indices.GetSize() / sizeof(uint32_t)),  // index count
        0u,                                     // instance count
        m_index_count,                          // first index
        static_cast<int32_t>(m_vertex_count),   // vertex offset
        0u                                      // first instance
    );

    command_buffer.CopyBufferToBuffer(vertices, *m_vertex_buffer, BufferToBufferCopyInfo{0, GetVertexBufferByteUsage(), vertices.GetSize()});
    command_buffer.CopyBufferToBuffer(indices, *m_index_buffer, BufferToBufferCopyInfo{0, GetIndexBufferByteUsage(), indices.GetSize()});

    m_vertex_count += static_cast<uint32_t>(vertices.GetSize() / sizeof(VertexData));
    m_index_count += static_cast<uint32_t>(indices.GetSize() / sizeof(uint32_t));

    return out_indirect_command;
}

template <typename VertexData, typename InstanceData>
void MeshManager<VertexData, InstanceData>::Reserve(
    CommandBuffer& command_buffer,
    const uint32_t reserve_vertices,
    const uint32_t reserve_indices,
    const uint32_t reserve_submesh)
{
    if (reserve_vertices > 0) {
        auto newVertexBuffer = RawBuffer::NewUnique(
            (GetVertexCount() + reserve_vertices) * sizeof(VertexData),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
            MemoryType::eDeviceLocal);

        command_buffer.CopyBufferToBuffer(
            *m_vertex_buffer,
            *newVertexBuffer,
            BufferToBufferCopyInfo{0, 0, GetVertexBufferByteUsage()});

        command_buffer.AddDeferredDestroyBuffer(m_vertex_buffer);
        m_vertex_buffer = std::move(newVertexBuffer);
    }

    if (reserve_indices > 0) {
        auto newIndexBuffer = RawBuffer::NewUnique(
            (GetIndexCount() + reserve_indices) * sizeof(uint32_t),
            vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
            MemoryType::eDeviceLocal);

        command_buffer.CopyBufferToBuffer(
            *m_index_buffer,
            *newIndexBuffer,
            BufferToBufferCopyInfo{0, 0, GetIndexBufferByteUsage()});

        command_buffer.AddDeferredDestroyBuffer(m_index_buffer);
        m_index_buffer = std::move(newIndexBuffer);
    }

    if (reserve_submesh > 0) {
        m_indirect_buffer.ReserveElement(command_buffer, reserve_submesh);
    }
}
