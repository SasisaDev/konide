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

    std::vector<const char*> exts {extensions};

    instanceInfo.enabledExtensionCount = exts.size();
    instanceInfo.ppEnabledExtensionNames = exts.data();

    std::vector<const char*> layrs {layers};

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
    indices = InternalFindQueueFamilies(physDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    if(indices.presentFamily.has_value()) 
    {   
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }
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

void KonideRenderer::CreateSwapchain(uint32_t width, uint32_t height)
{
    // ToDo: swapchain support info

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

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
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
}

void KonideRenderer::FlushRender()
{
    
}

KonideRenderer::~KonideRenderer()
{
    if (swapchain.swapchain) vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);
    if(surface) vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}