#include <konide/composition.h>

KonideComposition::KonideComposition(VkSurfaceKHR surface)
{

}

uint32_t KonideComposition::AddLayer(KonideLayer* proxy) {
    return 0;
}

void KonideComposition::Render(VkCommandBuffer commandBuffer, VkDevice device)
{
        for(KonideLayer* layer : Layers)
        {
            layer->Render(commandBuffer, device);
        }
}