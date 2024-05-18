#include "vm.h"
#include "mmanage.h"
#include <time.h>

#define BLK_ERROR_ID (1ull << 31)
#define CALLSTACK_SIZE (1ull << 22)

RuntimeBlock rBlks[1 << 16], *curRBlock;
CallFrame clStack[CALLSTACK_SIZE], *clStackTop;
uint32 rBlockCount = 0;

TrieNode vobjPathTrieRoot;

static inline uint16 getRealMdf(uint16 dtmdf) {
    return dtmdf >= gv0 ? min(clStackTop->genericTable[dtmdf - gv0], 10) : dtmdf;
}

#pragma region vobj loader
/// @brief load the runtime block using the vobj file whose path is PATH and return the block id of the runtime block that created
/// @param path the path of vobj file
/// @return the id of the block that created
uint32 loadRuntimeBlock(const char *path) {
    printf("loading rblock from file %s\n", path);
    FILE *fPtr = fopen(path, "rb");
    // create a new runtime block
    uint32 blkid = ++rBlockCount;
    RuntimeBlock *rblk = &rBlks[blkid];
    rblk->id = blkid;

    uint8 type; readData(fPtr, &type, uint8);

    // get the rely list
    readData(fPtr, &rblk->relyCount, uint64);
    rblk->relyBlkId = mallocArray(uint64, rblk->relyCount + 1);
    rblk->relyPath = mallocArray(char *, rblk->relyCount + 1);
    memset(rblk->relyBlkId, 0, sizeof(uint64) * (rblk->relyCount + 1));
    for (int i = 1; i <= rblk->relyCount; i++) rblk->relyPath[i] = readString(fPtr); 

    // ignore the definition
    ignoreString(fPtr);

    // get the type data for reflective system
    rblk->tdRoot = loadTypeData(fPtr);
    rblk->dataTmpl = generateDataTmpl(rblk->tdRoot);

    // read main offset
    readData(fPtr, &rblk->mainOffset, uint64);

    // read string list
    readData(fPtr, &rblk->strCount, uint64);
    rblk->strList = mallocArray(Object *, rblk->strCount);
    for (int i = 0; i < rblk->strCount; i++) {
        char *str = readString(fPtr);
        size_t strl = strlen(str);
        Object *strObj = newObject(strl + 1 + sizeof(uint64));
        strObj->refCount = strObj->rootRefCount = 1;
        *(uint64 *)strObj->data = sizeof(int8);
        memcpy(strObj->data + sizeof(uint64), str, strl + 1);
        free(str);
        rblk->strList[i] = strObj;
    }
    uint64 gloMemSize, vcodeSize;
    // read global memory size
    readData(fPtr, &gloMemSize, uint64);
    rblk->globalMemory = mallocArray(uint8, gloMemSize);
    memset(rblk->globalMemory, 0, sizeof(uint8) * gloMemSize);
    // read vcode
    readData(fPtr, &vcodeSize, uint64);
    rblk->vcode = mallocArray(uint8, vcodeSize);
    readArray(fPtr, rblk->vcode, uint8, vcodeSize);

    // insert the pointer of this runtime block into the vobj path trie
    Trie_insert(&vobjPathTrieRoot, path, rblk);

    return blkid;
}

/// @brief get the id of runtime block which is loaded from the vobj file whose path is PATH, if this vobj file has not been loaded, 
/// then the function will call loadRuntimeBlock(path) to load this vobj file.
/// @param path the path of vobj file
/// @return the id of runtime block
static inline uint32 getRBlockId(const char *path) {
    RuntimeBlock *blk = Trie_get(&vobjPathTrieRoot, path);
    if (blk != NULL) return blk->id;
    return loadRuntimeBlock(path);
}
#pragma endregion

void initVM() {
    Trie_init(&vobjPathTrieRoot);
    clStackTop = clStack;
    for (int i = 0; i < CALLSTACK_SIZE; i++) clStack[i].cStackTop = clStack[i].cStack;
    initGC();
}

static inline uint32 getRelyBlkId(int32 relyId) {
    if (!relyId) return curRBlock->id;
    if (!curRBlock->relyBlkId[relyId])
        curRBlock->relyBlkId[relyId] = getRBlockId(curRBlock->relyPath[relyId]);
    return curRBlock->relyBlkId[relyId];
}
/// @brief call the function in rBlocks[blkId], whose offset is OFFSET
/// @param blkId 
/// @param offset 
static inline void callFunc(uint32 blkId, uint64 offset) {
    clStackTop++;
    // keep in the same runtime block
    if (!blkId) clStackTop->blkId = (clStackTop - 1)->blkId;
    else { // switch to another runtime block
        clStackTop->blkId = blkId;
        curRBlock = &rBlks[clStackTop->blkId];
    }
    clStackTop->offset = offset;
    clStackTop->var = NULL;
    // printf("call function -> %u %llu\n", clStackTop->blkId, clStackTop->offset);
}

static inline void retFunc() {
    // clean the local variable list
    if (clStackTop->var != NULL) free(clStackTop->var);
    // clean the calculate stack
    clStackTop->cStackTop = clStackTop->cStack;
    clStackTop--;
    curRBlock = &rBlks[clStackTop->blkId];
    // printf("return function -> %u %llu\n", clStackTop->blkId, clStackTop->offset);
}

static inline void retFuncV() {
    // clean the local variable list
    if (clStackTop->var != NULL) free(clStackTop->var);
    // return the value in the top of calculate stack
    *(++((clStackTop - 1)->cStackTop)) = *clStackTop->cStackTop;
    // then do the same thing in retFunc()
    // clean the calculate stack
    clStackTop->cStackTop = clStackTop->cStack;
    clStackTop--;
    curRBlock = &rBlks[clStackTop->blkId];
    // printf("return function -> %u %llu value = %#018llx\n", clStackTop->blkId, clStackTop->offset, *clStackTop->cStackTop);
}

void debugInfo() {
    system("clear");
    // print call stack
    for (CallFrame *frm = clStack + 1; frm != clStackTop + 1; frm++) {
        printf("rely id = %4u, offset = %4llu ", frm->blkId, frm->offset);
        printf("gtable : [%d, %d, %d, %d, %d]\n",frm->genericTable[0], frm->genericTable[1], frm->genericTable[2], frm->genericTable[3], frm->genericTable[4]);
        printf("<CSTACK>\n");
        for (uint64 *cFrm = frm->cStack + 1; cFrm != frm->cStackTop + 1; cFrm++)
            printf("%#018llx\n", *cFrm);
    }
}
/// @brief the main loop of VM
void mainLoop() {
    static uint64 tmpData[16], argData[16];
    while (clStackTop != clStack) {
        if (checkGC()) genGC(0);
        uint32 vcode = *(uint32 *) &curRBlock->vcode[clStackTop->offset], tcmd = vcode & ((1 << 16) - 1);
        // printf("offset = %#018llx vcode = %x, tmd = %d\n", clStackTop->offset, vcode, tcmd);
        clStackTop->offset += sizeof(uint32);

        uint16  dtmdf1 = getRealMdf((vcode >> 16) & 15),
            dtmdf2 = getRealMdf((vcode >> 20) & 15);

        switch (tcmd) {
            case setgtbl : {
                uint64 size = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                for (int i = size - 1; i >= 0; i--)
                    (clStackTop + 1)->genericTable[i] = *(clStackTop->cStackTop--);
                break;
            }
            case setclgtbl: {
                uint64 offset = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                uint64 size = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                for (uint64 i = 0; i < size; i++)
                    clStackTop->genericTable[i] = *(uint64*)(((Object *)clStackTop->var[0])->data + offset + i * sizeof(uint64));
                break;
            }
            case getgtbl: {
                uint64 id = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                *(++clStackTop->cStackTop) = clStackTop->genericTable[id];
                break;
            }
            case getgtblsz: {
                static uint64 dtSz[] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8};
                uint64 id = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                *(++clStackTop->cStackTop) = dtSz[clStackTop->genericTable[id]];
                break;
            }
            default:
                printf("Unknown command : %x\n", vcode);
                return;
                break;
#pragma endregion
        }
        // debugInfo();
    }
}

int VM(const char *vobjPath) {
    initVM();

    // set the entry : the main address
    uint32 blk = loadRuntimeBlock(vobjPath);
    uint64 mainAddr = rBlks[blk].mainOffset;
    curRBlock = &rBlks[blk];
    // set entry
    callFunc(blk, mainAddr);

    clock_t st = clock();
    mainLoop();
    clock_t ed = clock();
    printf("\nexit, cost %f(s)\n", 1.0 * (1.0 * ed - 1.0 * st) / CLOCKS_PER_SEC);
    return 0;
}
