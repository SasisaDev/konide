#ifndef _KONIDE_LAYER_H
#define _KONIDE_LAYER_H

#include <cstdint>

class KonideProxy;

class KonideLayer
{
protected:

public:
    virtual uint32_t AddProxy(KonideProxy* proxy);
};

#endif