#ifndef __MMANAGE_H__
#define __MMANAGE_H__

#include "tools.h"
enum ObjectState {
    ObjectState_Active, ObjectState_Free, ObjectState_Waiting,
};
typedef struct tmpObject {
    uint64 dataSize, flagSize;
    uint64 *flag;
    uint8 *data;

    /// @brief the number of reference to this Object
    uint64 refCount, rootRefCount;

    int32 state;
    
    /// @brief the generation this Object belongs to
    uint32 genID;
} Object;

void initGC();
Object *newObject(uint64 size);
void refGC(Object *obj);
void genGC();

/// @brief check if it is necessary to run genGC()
/// @return 
int checkGC();

#endif