#ifndef _KONIDE_MATERIAL_H
#define _KONIDE_MATERIAL_H

#ifndef VK_NO_PROTOTYPES
#	define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <vector>

class KonideShader
{
protected:
    VkPipeline pipeline;
public:
    
};

class KonideMaterial
{
protected:
    
public:
    
};

class KonideMaterialFactory
{
public:
    static KonideMaterial* CreateMaterial(std::vector<char> vertexCode, std::vector<char> fragmentCode);
};

#endif