#include <konide/renderer.h>
#include "vulkan/vkloader.hpp"

#include <vector>
#include <string>
#include <stdexcept>
#include <set>

KonideRenderer::KonideRenderer(uint32_t RenderFeatures)
{
    RenderFeatureFlags = RenderFeatures;
}

KonideQueueFamilyIndices KonideRenderer::InternalFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    KonideQueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    VkBool32 presentSupport = false;
    uint32_t i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.graphicsFamily.has_value()) {
            indices.graphicsFamily = i;
        }

        if(surface != 0)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

VkPhysicalDevice KonideRenderer::InternalPickPhysDevice(std::vector<VkPhysicalDevice> &PhysicalDevices)
{
    for(int i = 0; i < PhysicalDevices.size(); i++)
    {
        if(InternalFindQueueFamilies(PhysicalDevices[i]).graphicsFamily.has_value())
        {
            return PhysicalDevices[i];
        }
    }

    throw std::runtime_error("Found no physical devices supporting Vulkan on current system.");
    return nullptr;
}

bool KonideRenderer::Initialize(std::vector<const char*> extensions, std::vector<const char*> layers)
{
    // Initialize Vulkan Loader
    VkResult result = vkloader::InitializeLoader();
    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not initialize Vulkan");
        return false;
    }

    // Create Vulkan Instance
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = 0;
    appInfo.pEngineName = "Konide";
    appInfo.pApplicationName = "Konide Application";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = 0;
    instanceInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> exts = InternalAssembleExtensions();
    exts.insert(exts.end(), extensions.begin(), extensions.end());

    instanceInfo.enabledExtensionCount = exts.size();
    instanceInfo.ppEnabledExtensionNames = exts.data();

    std::vector<const char*> layrs = InternalAssembleLayers();
    layrs.insert(layrs.end(), layers.begin(), layers.end());

    instanceInfo.enabledLayerCount = layrs.size();
    instanceInfo.ppEnabledLayerNames = layrs.data();

    result = vkloader::vkCreateInstance(&instanceInfo, nullptr, &instance);
    if(result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create VkInstance");
        return false;
    }

    // Pick Physical Device
    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physDevice = DelegatePickPhysDevice(devices);

    return true;
}

void KonideRenderer::SetSurface(VkSurfaceKHR newSurface)
{
    surface = newSurface;
}

void KonideRenderer::CreateDevice(std::vector<const char*> devExtensions, std::vector<const char*> devLayers)
{
    // Create Device Queue
    queueFamilyIndices = InternalFindQueueFamilies(physDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Create Device
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    std::vector<const char*> extensions = InternalAssembleDeviceExtensions();
    extensions.insert(extensions.end(), devExtensions.begin(), devExtensions.end());
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

    std::vector<const char*> layers = {};
    layers.insert(layers.end(), devLayers.begin(), devLayers.end());
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    deviceCreateInfo.ppEnabledLayerNames = layers.data();

    if (vkloader::vkCreateDevice(physDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device.");
    }

    vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    if(queueFamilyIndices.presentFamily.has_value()) 
    {   
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
    }
}

std::vector<const char*> KonideRenderer::InternalAssembleLayers()
{
    std::vector<const char*> layers;
    if(RenderFeatureFlags & KONIDE_RENDER_FEATURE_VALIDATION_LAYERS)
    {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    return layers;
}

std::vector<const char*> KonideRenderer::InternalAssembleExtensions()
{
    std::vector<const char*> exts;
    if(RenderFeatureFlags & KONIDE_RENDER_FEATURE_VALIDATION_LAYERS)
    {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return exts;
}

std::vector<const char*> KonideRenderer::InternalAssembleDeviceExtensions()
{
    std::vector<const char*> exts;
    if(RenderFeatureFlags & KONIDE_RENDER_FEATURE_SWAPCHAIN)
    {
        exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    return exts;
}

void KonideRenderer::CreateDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = callback;
    createInfo.pUserData = userData;

    vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
}

void KonideRenderer::CreateSwapchain(uint32_t width, uint32_t height)
{
    // ToDo: swapchain support info

    swapchain.swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

    std::vector<VkSurfaceFormatKHR> availableFormats;
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, nullptr);

    if (formatCount == 0) {
        throw std::runtime_error("No surface formats available");
    }

    availableFormats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, availableFormats.data());

    VkSurfaceFormatKHR resultFormat;
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            resultFormat = availableFormat;
        }
    }

    VkSurfaceFormatKHR surfaceFormat = resultFormat;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    swapchain.swapChainExtent = extent;

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.surface = surface;

    createInfo.minImageCount = 2;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value()};

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &(swapchain.swapchain)) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // Fetch images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, nullptr);
    swapchain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain.swapchain, &imageCount, swapchain.images.data());

    // Create image views
    swapchain.imageViews.resize(imageCount);

    for (size_t i = 0; i < swapchain.images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchain.images[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchain.swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchain.imageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views.");
        }
    }

    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.flags = 0;
    semaphoreCreateInfo.pNext = nullptr;

    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &aquireSemaphore);
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &submitSemaphore);

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    
    vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool);
}

void KonideRenderer::FlushRender()
{
    uint32_t imgIdx;
    vkAcquireNextImageKHR(device, swapchain.swapchain, 0, aquireSemaphore, 0, &imgIdx);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.commandPool = cmdPool;
    vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // Rendering Commands
    {
        VkClearColorValue color = { 1, 1, 0, 1 };
        VkImageSubresourceRange range = {};
        range.layerCount = 1;
        range.levelCount = 1;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCmdClearColorImage(cmdBuffer, swapchain.images[imgIdx], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, &color, 1, &range);
    }

    vkEndCommandBuffer(cmdBuffer);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;
    submitInfo.pSignalSemaphores = &submitSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &aquireSemaphore;
    submitInfo.waitSemaphoreCount = 1;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0);
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &imgIdx;
    presentInfo.pWaitSemaphores = &submitSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    vkQueuePresentKHR(presentQueue, &presentInfo);

    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);

}

KonideRenderer::~KonideRenderer()
{
    if(RenderFeatureFlags & KONIDE_RENDER_FEATURE_VALIDATION_LAYERS)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    for (auto imageView : swapchain.imageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    if (swapchain.swapchain) vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
    if(surface) vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}