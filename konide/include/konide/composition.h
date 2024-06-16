#ifndef _KONIDE_COMPOSITION_H
#define _KONIDE_COMPOSITION_H

#ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

class KonideLayer;
#include "layer.h"

struct KonideSwapchain {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkSurfaceKHR surface;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
};

class KonideComposition
{
protected:
    KonideSwapchain swapchain;
    std::vector<KonideLayer*> Layers;
public:
    KonideComposition(VkSurfaceKHR surface);

    virtual uint32_t AddLayer(KonideLayer* proxy);

    const KonideSwapchain& GetSwapchainInfo() const { return swapchain; }

    virtual void Render(VkCommandBuffer commandBuffer, VkDevice device);
};

#endif