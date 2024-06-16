#ifndef _KONIDE_RENDERER_H
#define _KONIDE_RENDERER_H

#include <vector>
#include <string>
#include <optional>

#include "composition.h"

#ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

enum EKonideRenderFeatureFlags
{
    KONIDE_RENDER_FEATURE_NONE = 0,
    KONIDE_RENDER_FEATURE_SWAPCHAIN = 1 << 0,
    KONIDE_RENDER_FEATURE_RAYTRACING = 1 << 1,
    KONIDE_RENDER_FEATURE_VALIDATION_LAYERS = 1 << 2
};

struct KonideQueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class KonideRenderer
{
private:
    std::vector<KonideComposition*> Compositions;

    VkInstance instance;
    VkPhysicalDevice physDevice;
    VkDevice device;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSemaphore aquireSemaphore;
    VkSemaphore submitSemaphore;
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;

    KonideSwapchain swapchain;

    KonideQueueFamilyIndices queueFamilyIndices;

    VkPhysicalDevice (*DelegatePickPhysDevice)(std::vector<VkPhysicalDevice> &PhysicalDevices) = &KonideRenderer::InternalPickPhysDevice;

    uint32_t RenderFeatureFlags = 0;

    static VkPhysicalDevice InternalPickPhysDevice(std::vector<VkPhysicalDevice> &PhysicalDevices);
    static KonideQueueFamilyIndices InternalFindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = 0);

    std::vector<const char*> InternalAssembleExtensions();
    std::vector<const char*> InternalAssembleLayers();
    
    std::vector<const char*> InternalAssembleDeviceExtensions();
    std::vector<const char*> InternalAssembleDeviceLayers();
protected:
    std::vector<KonideLayer*> Composition;

    void DestroySwapchain();

public:
    KonideRenderer(uint32_t RenderFeatures);
    ~KonideRenderer();

    bool Initialize(std::vector<const char*> extensions = {}, std::vector<const char*> layers = {});

    template <class LayerType>
    uint32_t CreateLayer(std::string name = "Default");
    uint32_t AddLayer(KonideLayer* layer);

    template <class LayerType>
    std::vector<KonideLayer*> GetLayersTyped();
    KonideLayer* GetLayer(uint32_t id);
    KonideLayer* GetLayer(std::string name);

    VkInstance GetInstance() const { return instance; }

    void SetPickPhysicalDeviceDelegate(VkPhysicalDevice (*NewDelegatePickPhysDevice)(std::vector<VkPhysicalDevice> &PhysicalDevices)) { DelegatePickPhysDevice = NewDelegatePickPhysDevice; }

    void SetSurface(VkSurfaceKHR newSurface) {swapchain.surface = newSurface;}

    uint32_t CreateComposition(VkSurfaceKHR surface);
    
    void CreateDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData);
    void CreateDevice(std::vector<const char*> devExtensions = {}, std::vector<const char*> devLayers = {}); 
    void CreateSwapchain(uint32_t width, uint32_t height);
    void RecreateSwapchain(uint32_t width, uint32_t height);

    void FlushRender();
};

template <class LayerType>
inline uint32_t KonideRenderer::CreateLayer(std::string name)
{
    KonideLayer* layer = new LayerType();
    Composition.push_back(layer);
    return Composition.size()-1;
}

#endif