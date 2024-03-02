#ifndef __MMANAGE_H__
#define __MMANAGE_H__

#include "tools.h"
#include "rflsys.h"
typedef enum {
    ObjectState_Active, ObjectState_Free, ObjectState_Waiting,
} ObjectState;

typedef struct tmpObject {
    uint64 dataSize, flagSize;
    uint64 *flag;
    uint8 *data;

    /// @brief the number of reference to this Object
    uint64 refCount, rootRefCount, crossRefCount;

    ObjectState state;
    
    /// @brief the generation this Object belongs to
    uint32 genId;

    ListElement *belong;

    ClassTypeData *typeData;
} Object;

uint64 getGenSize(int genId);
void initGC();
Object *newObject(uint64 size);
void refGC(Object *obj);
void genGC();

/// @brief check if it is necessary to run genGC()
/// @return 
int checkGC();

#endif