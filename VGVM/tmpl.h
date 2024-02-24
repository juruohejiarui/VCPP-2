#pragma once

#include "instruction.h"
#include "vgvm.h"

extern const uint32  
    refCntOffset,
    rootRefCntOffset, 
    objDataOffset,
    objGenOffset,
    relyBlkOffset,
    relyListOffset,
    entryListOffset,
    strListOffset;

#define blgBlkDisp ((uint32)(-(int32)8))
#define gtableDisp(x) ((uint32)(-(int32)(16 - (x))))
#define clStkDisp(x) ((uint32)(-(int32)(((x) - stkRegNumber) * 8 + 16)))
#define locVarDisp(x) ((uint32)(-(int32)((x + 1) * 8 + (stkSize - stkRegNumber) * 8 + 16)))

// actuallly the stkReg[0] will not be used and it is just a occupy symbol
extern const uint32 stkReg[], argReg[];
extern const uint32 stkRegNumber, stkSize, gtableSize;

#define CreateTemplInstList \
    InstList temp; \
    InstList_init(&temp);

InstList *getTgList();
void setTargetBlock(RuntimeBlock *tgBlk);

// insert the assembly code for preparing for calling a function in VM
void prepareCallVMFunc(InstList *insl, int isEntry, int stkTop);
void restoreFromCallVMFunc(InstList *insl, int isEntry, int stkTop);

void addVInst(uint64 offset, uint32 vinst, uint32 dtMdf1, uint32 dtMdf2, uint64 arg1, uint64 arg2);

void setInstBlk();
