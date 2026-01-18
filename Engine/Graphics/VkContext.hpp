#pragma once

#include "Utility/Types.hpp"
#include "Utility/Log.hpp"

#include <string>
#include <vector>

class Pipeline;
class Image;
class Swapchain;
class RenderPass;
class Descriptor;
class DescriptorSet;
class Shader;
class DeviceMemory;

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

constexpr uint32_t MaxFramesInFlight = 1;

class ENGINE_API VkContext {
    VkContext() = default;

    struct SupportedFeatures {
        bool fp16;
        bool io16;//storageInputOutput16
    } supportedFeatures{};
public:
    static VkContext& Get() {
        static VkContext instance;
        return instance;
    }

    void Init();
    void CleanUp();

    [[nodiscard]] const vk::raii::Instance& GetInstance() const { return instance; }
    [[nodiscard]] const vk::raii::SurfaceKHR& GetSurface() const { return surface; }
    [[nodiscard]] const vk::raii::Device& GetDevice() const { return device; }
    [[nodiscard]] const vk::raii::PhysicalDevice& GetPhysicalDevice() const { return physicalDevice; }
    [[nodiscard]] VmaAllocator GetVmaAllocator() const { return vmaAllocator; }

    [[nodiscard]] const vk::raii::Queue& GetGraphicsQueue() const { return graphicsQueue; }
    [[nodiscard]] const vk::raii::Queue& GetTransferQueue() const { return transferQueue; }
    [[nodiscard]] const vk::raii::Queue& GetComputeQueue() const { return computeQueue; }

    [[nodiscard]] uint32_t GetGraphicsQueueFamily() const { return graphicsQueueFamily; }
    [[nodiscard]] uint32_t GetTransferQueueFamily() const { return transferQueueFamily; }
    [[nodiscard]] uint32_t GetComputeQueueFamily() const { return computeQueueFamily; }

    [[nodiscard]] bool IsComputeQueueExists() const { return graphicsQueueFamily != -1; }
    [[nodiscard]] bool IsTransferQueueExists() const { return transferQueueFamily != -1; }

    [[nodiscard]] const vk::PhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_memoryProperties; }

    [[nodiscard]] vk::Queue GetQueue(const QueueType queueType) const {
        switch (queueType) {
        case QueueType::eGraphics:
            return graphicsQueue;
        case QueueType::eCompute:
            return computeQueue;
        case QueueType::eTransfer:
            return transferQueue;
        default:
            return graphicsQueue;
        }
    }

    [[nodiscard]] uint32_t GetQueueFamilyIndex(const QueueType queueType) const {
        switch (queueType) {
        case QueueType::eGraphics:
            return graphicsQueueFamily;
        case QueueType::eCompute:
            return computeQueueFamily;
        case QueueType::eTransfer:
            return transferQueueFamily;
        default:
            return graphicsQueueFamily;
        }
    }

    [[nodiscard]] const auto& GetSupportedFeatures() const { return supportedFeatures; }

    VkContext(const VkContext &) = delete;
    VkContext(VkContext &&) = delete;
    VkContext& operator=(const VkContext &) = delete;
    VkContext& operator=(VkContext &&) = delete;
private:
    vk::raii::Context context{};
    vk::raii::Instance instance{VK_NULL_HANDLE};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
    vk::raii::PhysicalDevice physicalDevice{VK_NULL_HANDLE};
    vk::raii::Device device{VK_NULL_HANDLE};
    vk::raii::SurfaceKHR surface{VK_NULL_HANDLE};

    uint32_t graphicsQueueFamily = -1;
    uint32_t computeQueueFamily = -1;
    uint32_t transferQueueFamily = -1;

    vk::raii::Queue graphicsQueue = VK_NULL_HANDLE;
    vk::raii::Queue transferQueue = VK_NULL_HANDLE;
    vk::raii::Queue computeQueue = VK_NULL_HANDLE;

    vk::PhysicalDeviceMemoryProperties m_memoryProperties{};

    VmaAllocator vmaAllocator = nullptr;

    void CreateInstance(std::vector<const char *> &extensions, std::vector<const char *> &layers);
    void CreateDebugMessenger();
    void CreateSurface();
    void CreateDevice(std::vector<const char *> &requireExtensions, std::vector<const char *> & optionalExtensions);
    void CreateQueues();
    void ChooseQueueFamilies();
    void CreateVmaAllocator();

    static std::string CheckInstanceLayerSupport(const std::vector<const char *> &layers);
    static std::string CheckInstanceExtensionSupport(const std::vector<const char *> &extensions);
    static std::string CheckDeviceExtensionSupport(vk::PhysicalDevice physicalDevice, const std::vector<const char *> &extensions);
    
    friend void FramebufferResizeCallback(struct GLFWwindow* window, int width, int height);
};