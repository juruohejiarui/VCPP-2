#ifndef __MMANAGE_H__
#define __MMANAGE_H__

#include "tools.h"
typedef struct tmpObject {
    uint64 dataSize, flagSize;
    uint64 *flag;
    uint8 *data;

    /// @brief the number of reference to this Object
    uint64 refCount, rootRefCount;
    
    /// @brief the generation this Object belongs to
    uint32 generatation;
} Object;

Object *newObject(uint64 size);
void refGC(Object *obj);
void genGC();

#endif