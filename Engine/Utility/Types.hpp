#pragma once

#include "Vulkan/vulkan.hpp"
#include "Vulkan/vulkan_raii.hpp"
#include "glm/glm.hpp"
#include "Utility/EngineDefines.hpp"
#include "Utility/Flag.hpp"

enum class ImageType : uint8_t {
    e1D = 0,
    e2D = 1,
    e3D = 2,
    eCube = 3,
};

enum class ImageUsageBit : uint8_t {
    eColorAttachment = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    eDepthAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    eStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    eDepthStencilAttachment = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    eTransferSrc = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    eTransferDst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    eSampled = VK_IMAGE_USAGE_SAMPLED_BIT,
    eStorage = VK_IMAGE_USAGE_STORAGE_BIT,
};

template<>
struct FlagTraits<ImageUsageBit> {
    static constexpr bool isBitmask = true;
};

using ImageUsageFlags = Flags<ImageUsageBit>;

enum class PipelineStageBit : uint64_t {
    eNone                             = VK_PIPELINE_STAGE_2_NONE,
    eTopOfPipe                        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
    eDrawIndirect                     = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
    eVertexInput                      = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
    eVertexShader                     = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
    eTessellationControlShader        = VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT,
    eTessellationEvaluationShader     = VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT,
    eGeometryShader                   = VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT,
    eFragmentShader                   = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
    eEarlyFragmentTests               = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
    eLateFragmentTests                = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
    eColorAttachmentOutput            = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    eComputeShader                    = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
    eAllTransfer                      = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
    eTransfer                         = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
    eBottomOfPipe                     = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    eHost                             = VK_PIPELINE_STAGE_2_HOST_BIT,
    eAllGraphics                      = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
    eAllCommands                      = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    eCopy                             = VK_PIPELINE_STAGE_2_COPY_BIT,
    eResolve                          = VK_PIPELINE_STAGE_2_RESOLVE_BIT,
    eBlit                             = VK_PIPELINE_STAGE_2_BLIT_BIT,
    eClear                            = VK_PIPELINE_STAGE_2_CLEAR_BIT,
    eIndexInput                       = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
    eVertexAttributeInput             = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT,
    ePreRasterizationShaders          = VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
};

template<>
struct FlagTraits<PipelineStageBit> {
    static constexpr bool isBitmask = true;
};

using PipelineStageFlags = Flags<PipelineStageBit>;

enum class ShaderStageBit : uint32_t {
    eVertex                 = VK_SHADER_STAGE_VERTEX_BIT,
    eTessellationControl    = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    eTessellationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    eGeometry               = VK_SHADER_STAGE_GEOMETRY_BIT,
    eFragment               = VK_SHADER_STAGE_FRAGMENT_BIT,
    eCompute                = VK_SHADER_STAGE_COMPUTE_BIT,
    eAllGraphics            = VK_SHADER_STAGE_ALL_GRAPHICS,
    eAll                    = VK_SHADER_STAGE_ALL,
};

template<>
struct FlagTraits<ShaderStageBit> {
    static constexpr bool isBitmask = true;
};

using ShaderStageFlags = Flags<ShaderStageBit>;

enum class AccessBit : uint64_t {
   eNone                                 = VK_ACCESS_2_NONE,
   eIndirectCommandRead                  = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
   eIndexRead                            = VK_ACCESS_2_INDEX_READ_BIT,
   eVertexAttributeRead                  = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
   eUniformRead                          = VK_ACCESS_2_UNIFORM_READ_BIT,
   eInputAttachmentRead                  = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT,
   eShaderRead                           = VK_ACCESS_2_SHADER_READ_BIT,
   eShaderWrite                          = VK_ACCESS_2_SHADER_WRITE_BIT,
   eColorAttachmentRead                  = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
   eColorAttachmentWrite                 = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
   eDepthStencilAttachmentRead           = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
   eDepthStencilAttachmentWrite          = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
   eTransferRead                         = VK_ACCESS_2_TRANSFER_READ_BIT,
   eTransferWrite                        = VK_ACCESS_2_TRANSFER_WRITE_BIT,
   eHostRead                             = VK_ACCESS_2_HOST_READ_BIT,
   eHostWrite                            = VK_ACCESS_2_HOST_WRITE_BIT,
   eMemoryRead                           = VK_ACCESS_2_MEMORY_READ_BIT,
   eMemoryWrite                          = VK_ACCESS_2_MEMORY_WRITE_BIT,
   eShaderSampledRead                    = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
   eShaderStorageRead                    = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
   eShaderStorageWrite                   = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
};

template<>
struct FlagTraits<AccessBit> {
    static constexpr bool isBitmask = true;
};

using AccessFlags = Flags<AccessBit>;

using ImageFormat = vk::Format;

constexpr uint8_t UndefinedLocationValue = -1;

struct DescriptorLocation {
    uint8_t set = UndefinedLocationValue;
    uint8_t binding = UndefinedLocationValue;

    //for unordered_map
    FORCE_INLINE bool operator==(const DescriptorLocation& other) const {
        return set == other.set && binding == other.binding;
    }

    FORCE_INLINE explicit operator std::string() const {
        return std::format("Descriptor Set: {}, Binding: {}", set, binding);
    }
};

//for unordered_map
//from chatgpt
template<>
struct std::hash<DescriptorLocation> {
    std::size_t operator()(const DescriptorLocation& dl) const noexcept {
        return (static_cast<std::size_t>(dl.set) << 8) | dl.binding;
    }
};

enum class MemoryType : uint8_t {
    eDeviceLocal, //Use VRAM or BAR as much as possible
    eCpuWrite, //Use BAR as much as possible
    eCpuReadWrite //Use System Ram
};

enum class QueueType {
    eGraphics,
    eCompute,
    eTransfer,
};

enum class PipelineBindPoint {
    eGraphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
    eCompute  = VK_PIPELINE_BIND_POINT_COMPUTE,
};

struct DepthStencilClear {
    float depthClear;
    uint32_t stencilClear;
};

enum class StorageBufferUsageType {
    eOnlyStorageBuffer = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,

    eIndirect = eOnlyStorageBuffer | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    eVertex = eOnlyStorageBuffer | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    eIndex = eOnlyStorageBuffer | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
};

enum class AttachmentLoadOp {
    eLoad     = VK_ATTACHMENT_LOAD_OP_LOAD,
    eClear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
    eDontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
};

enum class AttachmentStoreOp {
    eStore    = VK_ATTACHMENT_STORE_OP_STORE,
    eDontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    eNone     = VK_ATTACHMENT_STORE_OP_NONE,
};

enum class ImageResourceType {
    e2D        = VK_IMAGE_VIEW_TYPE_2D,
    e3D        = VK_IMAGE_VIEW_TYPE_3D,
    eCube      = VK_IMAGE_VIEW_TYPE_CUBE,
    e1DArray   = VK_IMAGE_VIEW_TYPE_1D_ARRAY,
    e2DArray   = VK_IMAGE_VIEW_TYPE_2D_ARRAY
};

struct ImageSubResource {
    ImageResourceType type{ImageResourceType::e2D};
    uint32_t baseMipmapLevel{0};
    uint32_t baseLayer{0};
    uint32_t mipmapLevelCount{1};
    uint32_t layerCount{1};

    friend bool operator==(const ImageSubResource& a, const ImageSubResource& b) {
        return a.type == b.type &&
               a.baseMipmapLevel == b.baseMipmapLevel &&
               a.baseLayer == b.baseLayer &&
               a.mipmapLevelCount == b.mipmapLevelCount &&
               a.layerCount == b.layerCount;
    }
};

//for unordered_map
//from chatgpt
template<>
struct std::hash<ImageSubResource> {
    std::size_t operator()(const ImageSubResource& s) const noexcept {
        auto h = std::hash<uint32_t>{}(static_cast<uint32_t>(s.type));
        h ^= std::hash<uint32_t>{}(s.baseMipmapLevel) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(s.baseLayer) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(s.mipmapLevelCount) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(s.layerCount) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct BufferImageCopyInfo {
    ImageSubResource subResource{};
    glm::vec3 imageExtent{0,0,0};
    uint32_t bufferOffset{};
    uint32_t imageOffset{};
};

struct ImageToImageCopyInfo {
    ImageSubResource srcSubResource{};
    ImageSubResource dstSubResource{};
    glm::vec3 srcOffset{0,0,0};
    glm::vec3 dstOffset{0,0,0};
    glm::vec3 extent{0,0,0};
};

struct BufferToBufferCopyInfo {
    uint32_t srcOffset{0};
    uint32_t dstOffset{0};
    uint64_t size{0};
};

struct PipelineBarrierInfo {
    PipelineStageBit srcStageFlags{PipelineStageBit::eNone};
    PipelineStageBit dstStageFlags{PipelineStageBit::eNone};
    AccessFlags srcAccessFlags{AccessBit::eNone};
    AccessFlags dstAccessFlags{AccessBit::eNone};
};

struct ImageBlitInfo {
    ImageSubResource srcSubResource{};
    ImageSubResource dstSubResource{};
    glm::vec2 srcOffset{0,0};
    glm::vec2 dstOffset{0,0};
    glm::vec2 srcExtent{0,0};
    glm::vec2 dstExtent{0,0};
};

struct ColorAttachmentInfo {
    glm::vec4 clearValue{0,0,0,0};
    vk::ImageView imageView;
    vk::ImageLayout imageLayout;
    AttachmentLoadOp loapOp = AttachmentLoadOp::eClear;
    AttachmentStoreOp storeOp = AttachmentStoreOp::eStore;
};

struct DepthAttachmentInfo {
    DepthStencilClear clearValue{1.f, 0};
    vk::ImageView imageView;
    vk::ImageLayout imageLayout;
    AttachmentLoadOp loapOp = AttachmentLoadOp::eClear;
    AttachmentStoreOp storeOp = AttachmentStoreOp::eStore;
};

struct DynamicRenderPassInfo {
    std::span<ColorAttachmentInfo> colorAttachments{};
    std::optional<DepthAttachmentInfo> depthAttachment{};
    glm::vec2 extent{0,0};
    glm::vec2 offset{0,0};
};

struct ImageCreateInfo {
    glm::uvec3 extent = {1, 1, 1};

    ImageFormat format = ImageFormat::eR8G8B8A8Unorm;
    ImageType type = ImageType::e2D;
    ImageUsageFlags usage = ImageUsageBit::eColorAttachment;
    MemoryType memoryUsage = MemoryType::eDeviceLocal;

    uint32_t arraySize = 1;
    uint32_t mipmapCount = 1;

    vk::ImageLayout initLayout = vk::ImageLayout::eGeneral;
};


struct DescriptorDesc {
    vk::ShaderStageFlags stage;
    uint32_t descriptorCount{};
    vk::DescriptorType type{};
    DescriptorLocation location;
};

struct ImageResource {
    ImageSubResource subresource{};
    class Image* image{};
    class Sampler* sampler = nullptr;//for only sampled image
};

struct NullData {};

template<typename T>
concept NotNull = (!std::is_same_v<T, NullData>);

template<typename T>
concept NotIntegral = (!std::is_integral_v<T>);

template<typename T>
concept IsClass = (!std::is_class_v<T>);

template<typename T>
concept NotVoid = (!std::is_same_v<T, void>);

template<typename T>
concept NotPointer = !std::is_pointer_v<T>;

template<typename T, typename M>
FORCE_INLINE constexpr uint32_t GetMemberSize([[maybe_unused]] M T::*member) {
    return sizeof(M);
}

template<typename T, typename M>
FORCE_INLINE constexpr std::size_t GetMemberOffset(M T::*member) {
    return *reinterpret_cast<uint64_t*>(&member);
}