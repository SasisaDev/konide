#ifndef _KONIDE_LAYER_H
#define _KONIDE_LAYER_H

#ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <cstdint>

class KonideProxy;

class KonideLayer
{
protected:

public:
    virtual uint32_t AddProxy(KonideProxy* proxy);

    virtual void Render(VkCommandBuffer commandBuffer, VkDevice device){}
};

#endif