#ifndef _KONIDE_SCENE_LAYER_H
#define _KONIDE_SCENE_LAYER_H

#include "../layer.h"
#include "KonideSceneProxy.h"

class KonideSceneLayer : public KonideLayer
{
protected:

public:
    virtual void Render(VkCommandBuffer cmd, VkDevice device) override;
};

#endif