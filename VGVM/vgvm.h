#pragma once
#include "instruction.h"
#include "../VM/mmanage.h"
#include "../VM/rflsys.h"

#define DefaultCallStackSize (1 << 20)

extern uint64 paramData[], gtable[], gtablesz[], VMflag;

typedef struct tmpRuntimeBlock {
    uint32 id;
    uint64 relyCnt;
    struct tmpRuntimeBlock **relyBlk;
    char **relyPath;
    uint64 strCnt;
    Object **strList;
    uint8 *gloMem;
    uint8 *vinstList;
    uint64 vinstListSize;
    uint64 gloMemSize;
    uint64 mainOffset;
    NamespaceTypeData *tdRoot;
    uint8 *dataTmpl;

    void **entryList;
    uint64 entryListSize;
    void *instBlk;
} RuntimeBlock;

typedef struct tmpCallFrame {
    uint64 calcStk[6];
    uint64 gtable[5];
    uint64 *varList;
    uint32 varListSize;
    RuntimeBlock *blgBlk;
} CallFrame;

CallFrame *getCallStackBottom();

#pragma region Interface for accessing the information and function of VM
void pauseVM(uint64 rbp, uint64 rsp);
uint64 callFunc(uint64 id);

void disconnectMember(Object *obj, Object *mem);
void connectMember(Object *obj, Object *mem, uint64 offset);

void *getGloAddr(RuntimeBlock *curBlk, uint64 id);

// get the blkId of the rely block of curBlk
uint64 getRelyId(RuntimeBlock *curBlk, uint64 id);
RuntimeBlock *getRuntimeBlock(uint64 blkId);
#pragma endregion

RuntimeBlock *loadRuntimeBlock(const char *vobjPath);

int launch(RuntimeBlock *rBlk);