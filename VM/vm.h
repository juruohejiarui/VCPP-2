#ifndef __VM_H__
#define __VM_H__

#include "tools.h"
#include "mmanage.h"
#include "rflsys.h"

typedef struct tmpRuntimeBlock {
    uint32 id;
    uint64 *relyBlkId, relyCount, strCount;
    char **relyPath;
    Object **strList;
    uint8 *dataTmpl, *globalMemory;
    uint8 *vcode;
    uint64 mainOffset;
    NamespaceTypeData *tdRoot;
} RuntimeBlock;

typedef struct tmpCallFrame { 
    uint64 offset;
    uint32 blkId;
    uint64 *var;
    uint8 genericTable[5];
    uint64 cStack[12], *cStackTop;
} CallFrame;

int VM(const char *vobjPath);
#endif