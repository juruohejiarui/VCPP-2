#include "vm.h"
#include "mmanage.h"

#define BLK_ERROR_ID (1ull << 31)
#define CALLSTACK_SIZE (1ull << 22)

RuntimeBlock rBlocks[1 << 16], *curRBlock;
CallFrame cStack[CALLSTACK_SIZE], *cStackTop;
uint32 rBlockCount = 0;

TrieNode vobjPathTrieRoot;

#pragma region vobj loader
/// @brief load the runtime block using the vobj file whose path is PATH and return the block id of the runtime block that created
/// @param path the path of vobj file
/// @return the id of the block that created
uint32 loadRuntimeBlock(const char *path) {
    FILE *fPtr = fopen(path, "rb");
    // create a new runtime block
    uint32 blkid = ++rBlockCount;
    RuntimeBlock *rblk = &rBlocks[blkid];
    rblk->id = blkid;

    uint8 type; readData(fPtr, &type, uint8);

    // get the type data for reflective system
    rblk->tdRoot = loadTypeData(fPtr);
    rblk->dataTemplate = generateDataTemplate(rblk->tdRoot);
    
    // get the rely list
    readData(fPtr, &rblk->relyCount, uint64);
    rblk->relyBlkId = mallocArray(uint64, rblk->relyCount + 1);
    rblk->relyList = mallocArray(char *, rblk->relyCount + 1);
    memset(rblk->relyBlkId, 0, sizeof(uint64) * (rblk->relyCount + 1));
    for (int i = 1; i <= rblk->relyCount; i++) rblk->relyList = readString(fPtr); 

    // ignore the definition
    ignoreString(fPtr);
    // read main offset
    readData(fPtr, &rblk->mainOffset, uint64);

    // read string list
    readData(fPtr, &rblk->strCount, uint64);
    rblk->strList = mallocArray(char *, rblk->strCount);
    for (int i = 0; i < rblk->strCount; i++) rblk->strList[i] = readString(fPtr);

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
uint32 getRBlockId(const char *path) {
    RuntimeBlock *blk = Trie_get(&vobjPathTrieRoot, path);
    if (blk != NULL) return blk->id;
    return loadRuntimeBlock(path);
}
#pragma endregion

void initVM() {
    Trie_init(&vobjPathTrieRoot);
    cStackTop = cStack;
    for (int i = 0; i < CALLSTACK_SIZE; i++) {
        cStack[i].calcStackTop = cStack[i].calcStack;
        cStack[i].calcOStackTop = cStack[i].calcOStack;
    }
    initGC();
}

uint32 getRelyBlkId(int32 relyId) {
    if (!curRBlock->relyBlkId[relyId])
        curRBlock->relyBlkId[relyId] = getRBlockId(curRBlock->relyList[relyId]);
    return curRBlock->relyBlkId[relyId];
}
void callFunc(uint32 relyId, uint64 offset) {
    cStackTop++;
    // keep in the same runtime block
    if (!relyId) cStackTop->blkId = (cStackTop - 1)->blkId;
    else { // switch to another runtime block
        cStackTop->blkId = getRelyBlkId(relyId);
        curRBlock = &rBlocks[cStackTop->blkId];
    }
    cStackTop->offset = offset;
    cStackTop->var = NULL;
}

void retFunc() {
    // clean the local variable list
    if (cStackTop->var != NULL) free(cStackTop->var);
    // clean the calculate stack
    cStackTop->calcStackTop = cStackTop->calcStack;
    cStackTop->calcOStackTop = cStackTop->calcOStack;
    cStackTop--;
    curRBlock = &rBlocks[cStackTop->blkId];
}

void retFuncV() {
    // clean the local variable list
    if (cStackTop->var != NULL) free(cStackTop->var);
    // return the value in the top of calculate stack
    *(++(cStackTop - 1)->calcStackTop) = *cStackTop->calcStackTop;
    // then do the same thing in retFunc()
    // clean the calculate stack
    cStackTop->calcStackTop = cStackTop->calcStack;
    cStackTop->calcOStackTop = cStackTop->calcOStack;
    cStackTop--;
    curRBlock = &rBlocks[cStackTop->blkId];
}

/// @brief get true value of the address in top of calculate stack using the modifiers
/// @param vlmdf value modifier
/// @param dtmdf data modifier
void getTrueValue(uint16 vlmdf, uint16 dtmdf) {
    if (vlmdf >= TrueVal) return ;
    if (vlmdf == MemberRef) {
        Object *fa = *(cStackTop->calcOStackTop--);
        fa->refCount--, fa->rootRefCount--;
        if (!fa->refCount) refGC(fa);
    }
    switch(dtmdf) {
        case i8:
        case u8:
            *(uint8 *)cStackTop->calcStackTop = *((uint8*)*cStackTop->calcStackTop);
            break;
        case i16:
        case u16:
            *(uint16 *)cStackTop->calcStackTop = *((uint16*)*cStackTop->calcStackTop);
            break;
        case i32:
        case u32:
            *(uint32 *)cStackTop->calcStackTop = *((uint32*)*cStackTop->calcStackTop);
            break;
        case i64:
        case u64:
            *(uint64 *)cStackTop->calcStackTop = *((uint64*)*cStackTop->calcStackTop);
            break;
        case o:
            *cStackTop->calcStackTop = (uint64)(Object *)*(uint64 *)*cStackTop->calcStackTop;
            break;
        case f32:
            *(float32 *)cStackTop->calcStackTop = *(float32 *)*cStackTop->calcStackTop;
            break;
        case f64:
            *(float64 *)cStackTop->calcStackTop = *(float64 *)*cStackTop->calcStackTop;
            break;
    }
}
/// @brief the main loop of VM
void mainLoop() {
    switch (cStackTop != cStack) {
        uint32 vcode = curRBlock->vcode[cStackTop->offset], tcmd = vcode & (1 << 16);
        cStackTop->offset += sizeof(uint32);

        uint16  dtmdf1 = (vcode >> 16) & 15, 
                dtmdf2 = (vcode >> 20) & 15,
                vlmdf1 = (vcode >> 24) & 4,
                vlmdf2 = (vcode >> 26) & 4;
        uint64 arg1, arg2;
        switch (tcmd) {
            case mov: {
                getTrueValue(vlmdf2, dtmdf1);
                Object *par = (vlmdf1 == VarRef ? NULL : *(cStackTop->calcOStackTop--));
                switch (dtmdf1) {
                    case i8:
                    case u8:
                        *(uint8 *)*(cStackTop->calcStackTop - 1) = *(uint8 *)cStackTop->calcStackTop;
                        break;
                    case i16:
                    case u16:
                        *(uint16 *)*(cStackTop->calcStackTop - 1) = *(uint16 *)cStackTop->calcStackTop;
                        break;
                    case i32:
                    case u32:
                        *(uint32 *)*(cStackTop->calcStackTop - 1) = *(uint32 *)cStackTop->calcStackTop;
                        break;
                    case i64:
                    case u64:
                        *(uint64 *)*(cStackTop->calcStackTop - 1) = *(uint64 *)cStackTop->calcStackTop;
                        break;
                    case o: {
                        Object *lst = (Object *)*(uint64 *)*(cStackTop->calcStackTop - 1);
                        if (lst) {
                            if (vlmdf1 == VarRef) {
                                lst->refCount -= 2, lst->rootRefCount -= 2;
                            } else {
                                lst->refCount -= 2, lst->rootRefCount--;
                                lst->crossRefCount -= (lst->genId < par->genId);
                            }
                            if (!lst->refCount) refGC(lst);
                        }
                        Object *nObj = (Object *)*cStackTop->calcStackTop;
                        if (nObj) {
                            if (vlmdf1 == MemberRef) {
                                nObj->rootRefCount--;
                                nObj->crossRefCount += (nObj->genId < par->genId);
                            }
                        }
                        *(uint64 *)*(cStackTop->calcStackTop - 1) = nObj;
                    }
                }
                par->rootRefCount--, par->refCount--;
                if (!par->refCount) refGC(par);
                break;
            }
            case add: {
                getTrueValue(vlmdf2, dtmdf1), getTrueValue(vlmdf1, dtmdf1);
                uint64 dt = *(cStackTop->calcStackTop--);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *)cStackTop->calcStackTop += *(int8 *)dt;
                        break;
                    case u8:
                        *(uint8 *)cStackTop->calcStackTop += *(uint8 *)dt;
                        break;
                    case i16:
                        *(int16 *)cStackTop->calcStackTop += *(int16 *)dt;
                        break;
                    case u16:
                        *(uint16 *)cStackTop->calcStackTop += *(uint16 *)dt;
                        break;
                    case i32:
                        *(int32 *)cStackTop->calcStackTop += *(int32 *)dt;
                        break;
                    case u32:
                        *(uint32 *)cStackTop->calcStackTop += *(uint32 *)dt;
                        break;
                    case i64:
                        *(int8 *)cStackTop->calcStackTop += *(int64 *)dt;
                        break;
                    case u64:
                        *(uint8 *)cStackTop->calcStackTop += *(uint64 *)dt;
                        break;
                    case f32:
                        *(float32 *)cStackTop->calcStackTop += *(float32 *)dt;
                        break;
                    case f64:
                        *(float64 *)cStackTop->calcStackTop += *(float64 *)dt;
                        break;
                }
                break;
            }
            case sub: {
                getTrueValue(vlmdf2, dtmdf1), getTrueValue(vlmdf1, dtmdf1);
                uint64 dt = *(cStackTop->calcStackTop--);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *)cStackTop->calcStackTop -= *(int8 *)dt;
                        break;
                    case u8:
                        *(uint8 *)cStackTop->calcStackTop -= *(uint8 *)dt;
                        break;
                    case i16:
                        *(int16 *)cStackTop->calcStackTop -= *(int16 *)dt;
                        break;
                    case u16:
                        *(uint16 *)cStackTop->calcStackTop -= *(uint16 *)dt;
                        break;
                    case i32:
                        *(int32 *)cStackTop->calcStackTop -= *(int32 *)dt;
                        break;
                    case u32:
                        *(uint32 *)cStackTop->calcStackTop -= *(uint32 *)dt;
                        break;
                    case i64:
                        *(int8 *)cStackTop->calcStackTop -= *(int64 *)dt;
                        break;
                    case u64:
                        *(uint8 *)cStackTop->calcStackTop -= *(uint64 *)dt;
                        break;
                    case f32:
                        *(float32 *)cStackTop->calcStackTop -= *(float32 *)dt;
                        break;
                    case f64:
                        *(float64 *)cStackTop->calcStackTop -= *(float64 *)dt;
                        break;
                }
                break;
            }
            case mul: {
                getTrueValue(vlmdf2, dtmdf1), getTrueValue(vlmdf1, dtmdf1);
                uint64 dt = *(cStackTop->calcStackTop--);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *)cStackTop->calcStackTop *= *(int8 *)dt;
                        break;
                    case u8:
                        *(uint8 *)cStackTop->calcStackTop *= *(uint8 *)dt;
                        break;
                    case i16:
                        *(int16 *)cStackTop->calcStackTop *= *(int16 *)dt;
                        break;
                    case u16:
                        *(uint16 *)cStackTop->calcStackTop *= *(uint16 *)dt;
                        break;
                    case i32:
                        *(int32 *)cStackTop->calcStackTop *= *(int32 *)dt;
                        break;
                    case u32:
                        *(uint32 *)cStackTop->calcStackTop *= *(uint32 *)dt;
                        break;
                    case i64:
                        *(int8 *)cStackTop->calcStackTop *= *(int64 *)dt;
                        break;
                    case u64:
                        *(uint8 *)cStackTop->calcStackTop *= *(uint64 *)dt;
                        break;
                    case f32:
                        *(float32 *)cStackTop->calcStackTop *= *(float32 *)dt;
                        break;
                    case f64:
                        *(float64 *)cStackTop->calcStackTop *= *(float64 *)dt;
                        break;
                }
                break;
            }
            case _div: {
                getTrueValue(vlmdf2, dtmdf1), getTrueValue(vlmdf1, dtmdf1);
                uint64 dt = *(cStackTop->calcStackTop--);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *)cStackTop->calcStackTop /= *(int8 *)dt;
                        break;
                    case u8:
                        *(uint8 *)cStackTop->calcStackTop /= *(uint8 *)dt;
                        break;
                    case i16:
                        *(int16 *)cStackTop->calcStackTop /= *(int16 *)dt;
                        break;
                    case u16:
                        *(uint16 *)cStackTop->calcStackTop /= *(uint16 *)dt;
                        break;
                    case i32:
                        *(int32 *)cStackTop->calcStackTop /= *(int32 *)dt;
                        break;
                    case u32:
                        *(uint32 *)cStackTop->calcStackTop /= *(uint32 *)dt;
                        break;
                    case i64:
                        *(int8 *)cStackTop->calcStackTop /= *(int64 *)dt;
                        break;
                    case u64:
                        *(uint8 *)cStackTop->calcStackTop /= *(uint64 *)dt;
                        break;
                    case f32:
                        *(float32 *)cStackTop->calcStackTop /= *(float32 *)dt;
                        break;
                    case f64:
                        *(float64 *)cStackTop->calcStackTop /= *(float64 *)dt;
                        break;
                }
                break;
            }
        }
    }
}

int VM(const char *vobjPath) {
    initVM();

    // set the entry : the main address
    uint32 blk = loadRuntimeBlock(vobjPath);
    uint64 mainAddr = rBlocks[blk].mainOffset;
    curRBlock = &rBlocks[blk];
    // set entry
    callFunc(blk, mainAddr);

    mainLoop();
    return 0;
}