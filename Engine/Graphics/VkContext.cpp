#include "Graphics/VkContext.hpp"

#include "Graphics/Descriptor.hpp"
#include "Utility/EngineDefines.hpp"
#include "Engine.hpp"

#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <SDL3/SDL_vulkan.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <ranges>
#include <vector>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData) {

    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LogWarning("{}", pCallbackData->pMessage);

    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        LogWarning("{}", pCallbackData->pMessage);

    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        LogInfo("{}", pCallbackData->pMessage);

    return VK_FALSE;
}

void VkContext::Init() {
    std::vector<const char*> requireInstanceExtensions = Window::GetVulkanExtensions();
    if constexpr (debug) requireInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char*> requireLayers{};
    if constexpr (debug) requireLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    if constexpr (debug) requireLayers.emplace_back("VK_LAYER_KHRONOS_synchronization2");
    //if constexpr (debug) requireLayers.emplace_back("VK_LAYER_LUNARG_gfxreconstruct");

    std::vector<const char*> requireDeviceExtensions{};
    requireDeviceExtensions.emplace_back("VK_KHR_swapchain");
    requireDeviceExtensions.emplace_back("VK_EXT_memory_budget");
    requireDeviceExtensions.emplace_back("VK_EXT_robustness2");
    requireDeviceExtensions.emplace_back("VK_EXT_pageable_device_local_memory");
    requireDeviceExtensions.emplace_back("VK_EXT_memory_priority");

    std::vector<const char*> optionalDeviceExtensions{};

    auto unsupportedLayers  = CheckInstanceLayerSupport(requireLayers);
    if (!unsupportedLayers.empty()) LogError("{}", unsupportedLayers);

    auto unsupportedExtensions = CheckInstanceExtensionSupport(requireInstanceExtensions);
    if (!unsupportedExtensions.empty()) LogError("{}", unsupportedExtensions);
    
    CreateInstance(requireInstanceExtensions, requireLayers);
    if constexpr (debug) CreateDebugMessenger();
    CreateSurface();
    CreateDevice(requireDeviceExtensions, optionalDeviceExtensions);
    CreateQueues();
    CreateVmaAllocator();
}

void VkContext::CleanUp() {
    if (vmaAllocator)
        vmaDestroyAllocator(vmaAllocator);

    graphicsQueue.clear();
    computeQueue.clear();
    transferQueue.clear();
    surface.clear();
    device.clear();
    physicalDevice.clear();
    debugMessenger.clear();
    instance.clear();
}

void VkContext::CreateInstance(std::vector<const char *>& extensions, std::vector<const char *>& layers) {
    vk::ApplicationInfo applicationInfo;
    applicationInfo.setApplicationVersion(VK_MAKE_VERSION(0, 1, 0))
    .setApiVersion(VK_API_VERSION_1_3)
    .setEngineVersion(VK_MAKE_VERSION(0, 1, 0))
    .setPEngineName("Dnm Engine")
    .setPApplicationName("dnm");

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if constexpr (debug) {
        debugCreateInfo.setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning 
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(DebugCallback);
    }

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.setEnabledExtensionCount(extensions.size())
    .setEnabledLayerCount(layers.size())
    .setPEnabledExtensionNames(extensions)
    .setPEnabledLayerNames(layers)
    .setPApplicationInfo(&applicationInfo)
    ;

    if constexpr (debug) instanceCreateInfo.setPNext(&debugCreateInfo);

    instance = vk::raii::Instance(context, instanceCreateInfo);
}

void VkContext::CreateDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity
     = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;

    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo);
}

void VkContext::CreateSurface() {
    VkSurfaceKHR surface_C;

    if (!SDL_Vulkan_CreateSurface(GetWindow().GetSDLWindow(), *instance, nullptr, &surface_C))
        LogError("failed to create window surface!, SDL_Error: {}", SDL_GetError());

    surface = vk::raii::SurfaceKHR(instance, surface_C);
}

void VkContext::CreateDevice(std::vector<const char*>& requireExtensions, std::vector<const char *> & optionalExtensions) {
    auto devices = instance.enumeratePhysicalDevices();
    
    if (devices.empty())
        LogError("failed to find GPUs with Vulkan support!");

    for (auto& _device : devices) {
        if (!CheckDeviceExtensionSupport(_device, requireExtensions).empty())
            continue;
        physicalDevice = _device;
    }
    
    if (*physicalDevice == VK_NULL_HANDLE) {
        std::string ext;
        for (auto extension : requireExtensions)
            ext += std::string(extension) + "\n";
        LogError("your gpu unsupported this extensions: {}", requireExtensions);
    }

    auto unsupportedOptExts = CheckDeviceExtensionSupport(physicalDevice, optionalExtensions);
    if (!unsupportedOptExts.empty())
        LogWarning("unsupported optional extensions: {}", unsupportedOptExts);

    vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableMemoryFeatures;

    vk::PhysicalDeviceRobustness2FeaturesEXT robustnessFeaturesEXT{};

    robustnessFeaturesEXT.pNext = &pageableMemoryFeatures;
    vk::PhysicalDeviceVulkan11Features features11{};
    features11.pNext = &robustnessFeaturesEXT;
    vk::PhysicalDeviceVulkan12Features features12{};
    features12.pNext = &features11;
    vk::PhysicalDeviceVulkan13Features features13{};
    features13.pNext = &features12;
    vk::PhysicalDeviceFeatures2 deviceFeatures{};
    
    deviceFeatures.pNext = &features13;
    //(*physicalDevice).getFeatures2(&deviceFeatures);

    //renderdoc problems, some device dosn't support capture replay
    features12.bufferDeviceAddress = vk::True;
    features11.storageInputOutput16 = vk::True;
    features12.shaderFloat16 = vk::True;
    features13.synchronization2 = vk::True;
    features13.dynamicRendering = vk::True;
    features12.descriptorIndexing = vk::True;
    features12.hostQueryReset = vk::True;
    features12.drawIndirectCount = vk::True;
    features11.shaderDrawParameters = vk::True;
    features12.vulkanMemoryModel = vk::True;
    deviceFeatures.features.pipelineStatisticsQuery = vk::True;
    deviceFeatures.features.samplerAnisotropy = vk::True;
    deviceFeatures.features.robustBufferAccess = vk::True;
    robustnessFeaturesEXT.nullDescriptor = vk::True;
    robustnessFeaturesEXT.robustBufferAccess2 = vk::True;
    robustnessFeaturesEXT.robustImageAccess2 = vk::True;

    if (!features11.storageInputOutput16)
        LogWarning("this device dosen't have storageInputOutput16 support");
    supportedFeatures.io16 = features11.storageInputOutput16;

    if (!features12.shaderFloat16)
        LogWarning("this device dosen't have shaderFloat16 support");
    supportedFeatures.fp16 = features12.shaderFloat16;

    ChooseQueueFamilies();

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::vector<uint32_t> uniqueQueueFamilies = {graphicsQueueFamily};
    if (computeQueueFamily != -1) uniqueQueueFamilies.emplace_back(computeQueueFamily);
    if (computeQueueFamily != -1) uniqueQueueFamilies.emplace_back(transferQueueFamily);

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setPQueuePriorities(&queuePriority)
                        .setQueueFamilyIndex(queueFamily)
                        .setQueueCount(1);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.setQueueCreateInfos(queueCreateInfos)
                    .setEnabledExtensionCount(requireExtensions.size())
                    .setPEnabledExtensionNames(requireExtensions)
                    .setPNext(&deviceFeatures);

    try {
        device = physicalDevice.createDevice(deviceCreateInfo);
    } 
    catch (const std::exception& e) {
        LogError("{}", e.what());
    }

    m_memoryProperties = physicalDevice.getMemoryProperties();
}

void VkContext::CreateQueues() {
    vk::DeviceQueueInfo2 queueInfo;
    queueInfo.setQueueFamilyIndex(graphicsQueueFamily)
                .setQueueIndex(0);

    graphicsQueue = device.getQueue2(queueInfo);

    if (computeQueueFamily != -1) {
        queueInfo.setQueueFamilyIndex(computeQueueFamily)
                        .setQueueIndex(0);

        computeQueue = device.getQueue2(queueInfo);
    }

    if (transferQueueFamily != -1) {
        queueInfo.setQueueFamilyIndex(transferQueueFamily)
                        .setQueueIndex(0);

        transferQueue = device.getQueue2(queueInfo);
    }
}

void VkContext::CreateVmaAllocator() {
    VmaAllocatorCreateInfo createInfo{};
    createInfo.instance = *instance;
    createInfo.device = *device;
    createInfo.physicalDevice = *physicalDevice;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.flags = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                        | VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&createInfo, &vmaAllocator);

    if (auto result = vmaCreateAllocator(&createInfo, &vmaAllocator); static_cast<vk::Result>(result) != vk::Result::eSuccess)
        LogError("vma allocator failed to create, Error: {}", vk::to_string(static_cast<vk::Result>(result)));
}

void VkContext::ChooseQueueFamilies() {
    const auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            graphicsQueueFamily = i;

        else if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute
                && computeQueueFamily == -1
                // To make it different from graphics queue
                && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
            computeQueueFamily = i;

        else if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer
                && transferQueueFamily == -1
                // To make it different from graphics queue and compute
                && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
                && !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute))
            transferQueueFamily = i;

        i++;
    }
}

std::string VkContext::CheckInstanceLayerSupport(const std::vector<const char*>& layers) {
    std::string unsupportedLayers;
    const auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (auto& layer : layers) {
        bool isFound = false;
        for (auto& availableLayer : availableLayers) {
            if (std::string(layer) == availableLayer.layerName)
                isFound = true;
        }
        if (isFound == false) {
            unsupportedLayers += layer;
            unsupportedLayers += "\n";
        }
    }

    return unsupportedLayers;
}

std::string VkContext::CheckInstanceExtensionSupport(const std::vector<const char*>& extensions) {
    std::string unsupportedExtensions;
    const auto availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (auto& extension : extensions) {
        bool isFound = false;
        for (auto& availableExtension : availableExtensions) {
            if (std::string(extension) == availableExtension.extensionName)
                isFound = true;
        }
        if (isFound == false) {
            unsupportedExtensions += extension;
            unsupportedExtensions += "\n";
        }
    }

    return unsupportedExtensions;
}

std::string VkContext::CheckDeviceExtensionSupport(const vk::PhysicalDevice physicalDevice, const std::vector<const char*>& extensions) {
    std::string unsupportedExtensions;
    const auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (auto& extension : extensions) {
        bool isFound = false;
        for (auto& availableExtension : availableExtensions) {
            if (std::string(extension) == availableExtension.extensionName)
                isFound = true;
        }
        if (isFound == false) {
            unsupportedExtensions += "\n";
            unsupportedExtensions += extension;
        }
    }

    return unsupportedExtensions;
}