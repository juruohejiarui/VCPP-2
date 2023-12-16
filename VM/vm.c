#include "vm.h"

#define BLK_ERROR_ID (1u << 31)
#define CALLSTACK_SIZE (1 << 24)

RuntimeBlock rBlocks[1024];
CallFrame cStack[CALLSTACK_SIZE];
int rBlockCount = 0;

#pragma region vobj loader
uint32 loadRuntimeBlock(const char *path) {
    FILE *fPtr = fopen(path, "rb");
    int blkid = ++rBlockCount;
    RuntimeBlock *rblk = &rBlocks[blkid];
    uint8 type; readData(fPtr, &type, uint8);

    // get the type data for reflective system
    rblk->tdRoot = loadTypeData(fPtr);
    rblk->dataTemplate = generateDataTemplate(rblk->tdRoot);
    
    // get the rely list
    readData(fPtr, rblk->relyCount, uint32);
    rblk->relyBlkId = mallocArray(uint32, rblk->relyCount + 1);
    rblk->relyList = mallocArray(char *, rblk->relyCount + 1);
    memset(rblk->relyBlkId, 0, sizeof(uint32) * (rblk->relyCount + 1));
    for (int i = 1; i <= rblk->relyCount; i++) rblk->relyList = readString(fPtr); 

    // ignore the definition
    ignoreString(fPtr);

}
#pragma endregion

int VM(const char *vobjPath) {

}