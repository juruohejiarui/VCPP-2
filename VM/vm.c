#include "vm.h"
#include "mmanage.h"
#include <time.h>

#define BLK_ERROR_ID (1ull << 31)
#define CALLSTACK_SIZE (1ull << 22)

RuntimeBlock rBlocks[1 << 16], *curRBlock;
CallFrame clStack[CALLSTACK_SIZE], *clStackTop;
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
    for (int i = 1; i <= rblk->relyCount; i++) rblk->relyList[i] = readString(fPtr); 

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
static inline uint32 getRBlockId(const char *path) {
    RuntimeBlock *blk = Trie_get(&vobjPathTrieRoot, path);
    if (blk != NULL) return blk->id;
    return loadRuntimeBlock(path);
}
#pragma endregion

void initVM() {
    Trie_init(&vobjPathTrieRoot);
    clStackTop = clStack;
    for (int i = 0; i < CALLSTACK_SIZE; i++) {
        clStack[i].cStackTop = clStack[i].cStack;
        clStack[i].oStackTop = clStack[i].oStack;
    }
    initGC();
}

static inline uint32 getRelyBlkId(int32 relyId) {
    if (!relyId) return curRBlock->id;
    if (!curRBlock->relyBlkId[relyId])
        curRBlock->relyBlkId[relyId] = getRBlockId(curRBlock->relyList[relyId]);
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
        curRBlock = &rBlocks[clStackTop->blkId];
    }
    clStackTop->offset = offset;
    clStackTop->var = NULL;
}

static inline void retFunc() {
    // clean the local variable list
    if (clStackTop->var != NULL) free(clStackTop->var);
    // clean the calculate stack
    clStackTop->cStackTop = clStackTop->cStack;
    clStackTop->oStackTop = clStackTop->oStack;
    clStackTop--;
    curRBlock = &rBlocks[clStackTop->blkId];
}

static inline void retFuncV() {
    // clean the local variable list
    if (clStackTop->var != NULL) free(clStackTop->var);
    // return the value in the top of calculate stack
    *(++((clStackTop - 1)->cStackTop)) = *clStackTop->cStackTop;
    // then do the same thing in retFunc()
    // clean the calculate stack
    clStackTop->cStackTop = clStackTop->cStack;
    clStackTop->oStackTop = clStackTop->oStack;
    clStackTop--;
    curRBlock = &rBlocks[clStackTop->blkId];
}

inline uint16 getRealMdf(uint16 dtmdf) {
    return dtmdf >= gv0 ? min(clStackTop->genericTable[dtmdf - gv0], 10) : dtmdf;
}
/// @brief get true value of the address in top of calculate stack using the modifiers
/// @param vlmdf value modifier
/// @param dtmdf data modifier
static inline void getTrueValue(uint16 vlmdf, uint16 dtmdf) {
    if (vlmdf >= TrueVal) return ;
    switch(dtmdf) {
        case i8:
        case u8:
            *clStackTop->cStackTop = *((uint8*)*clStackTop->cStackTop) & ((1ull << 8) - 1);
            break;
        case i16:
        case u16:
            *clStackTop->cStackTop = *((uint16*)*clStackTop->cStackTop) & ((1ull << 16) - 1);
            break;
        case i32:
        case u32:
            *clStackTop->cStackTop = *((uint32*)*clStackTop->cStackTop) & ((1ull << 32) - 1);
            break;
        case i64:
        case u64:
            *(uint64 *)clStackTop->cStackTop = *((uint64*)*clStackTop->cStackTop);
            break;
        case o:
            *clStackTop->cStackTop = (uint64)(Object *)*(uint64 *)*clStackTop->cStackTop;
            break;
        case f32:
            *(float32 *)clStackTop->cStackTop = *(float32 *)*clStackTop->cStackTop;
            *clStackTop->cStackTop &= ((1ull << 32) - 1);
            break;
        case f64:
            *(float64 *)clStackTop->cStackTop = *(float64 *)*clStackTop->cStackTop;
            break;
    }
    if (vlmdf == MemberRef) {
        Object *fa = *(clStackTop->oStackTop--);
        fa->refCount--, fa->rootRefCount--;
        if (!fa->refCount) refGC(fa);
    }
}


/// @brief the main loop of VM
void mainLoop() {
    static uint64 tmpData[16], argData[16];
    while (clStackTop != clStack) {
        if (checkGC()) genGC();
        uint32 vcode = *(uint32 *) &curRBlock->vcode[clStackTop->offset], tcmd = vcode & ((1 << 16) - 1);
        // printf("offset = %#018llx vcode = %x, tmd = %d\n", clStackTop->offset, vcode, tcmd);
        clStackTop->offset += sizeof(uint32);

        uint16  dtmdf1 = getRealMdf((vcode >> 16) & 15),
            dtmdf2 = getRealMdf((vcode >> 20) & 15),
            vlmdf1 = (vcode >> 24) & 3,
            vlmdf2 = (vcode >> 26) & 3;

        // printf("modifiers : %d %d %d %d\n", dtmdf1, dtmdf2, vlmdf1, vlmdf2);
        uint64 arg1, arg2;
        switch (tcmd) {
            case mov: {
                getTrueValue(vlmdf2, dtmdf1);
                Object *par = (vlmdf1 == VarRef ? NULL : *(clStackTop->oStackTop--));
                switch (dtmdf1) {
                    case i8:
                    case u8:
                        *(uint8 *) *(clStackTop->cStackTop - 1) = *(uint8 *) clStackTop->cStackTop;
                        break;
                    case i16:
                    case u16:
                        *(uint16 *) *(clStackTop->cStackTop - 1) = *(uint16 *) clStackTop->cStackTop;
                        break;
                    case i32:
                    case u32:
                        *(uint32 *) *(clStackTop->cStackTop - 1) = *(uint32 *) clStackTop->cStackTop;
                        break;
                    case i64:
                    case u64:
                        *(uint64 *) *(clStackTop->cStackTop - 1) = *(uint64 *) clStackTop->cStackTop;
                        break;
                    case f32:
                        *(float32 *) *(clStackTop->cStackTop - 1) = *(float32 *) clStackTop->cStackTop;
                        break;
                    case f64:
                        *(float64 *) *(clStackTop->cStackTop - 1) = *(float64 *) clStackTop->cStackTop;
                        break;
                    case o: {
                        Object *lst = (Object *) *(uint64 *) *(clStackTop->cStackTop - 1);
                        if (lst) {
                            if (vlmdf1 == VarRef) {
                                lst->refCount -= 2, lst->rootRefCount -= 2;
                            }
                            else {
                                lst->refCount -= 2, lst->rootRefCount--;
                                lst->crossRefCount -= (lst->genId < par->genId);
                            }
                            if (!lst->refCount) refGC(lst);
                        }
                        Object *nObj = (Object *) *clStackTop->cStackTop;
                        if (nObj) {
                            if (vlmdf1 == MemberRef) {
                                nObj->rootRefCount--;
                                nObj->crossRefCount += (nObj->genId < par->genId);
                            }
                        }
                        *(uint64 *) *(clStackTop->cStackTop - 1) = (uint64) nObj;
                    }
                }
                clStackTop->cStackTop -= 2;
                if (par) {
                    par->rootRefCount--, par->refCount--;
                    if (!par->refCount) refGC(par);
                }
                break;
            }
            case gvl: {
                getTrueValue(vlmdf1, dtmdf1);
                break;
            }

#pragma region basic operator (+ - * / % << >> & | ^ ~)
            case add: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *) clStackTop->cStackTop += *(int8 *) &dt;
                        break;
                    case u8:
                        *(uint8 *) clStackTop->cStackTop += *(uint8 *) &dt;
                        break;
                    case i16:
                        *(int16 *) clStackTop->cStackTop += *(int16 *) &dt;
                        break;
                    case u16:
                        *(uint16 *) clStackTop->cStackTop += *(uint16 *) &dt;
                        break;
                    case i32:
                        *(int32 *) clStackTop->cStackTop += *(int32 *) &dt;
                        break;
                    case u32:
                        *(uint32 *) clStackTop->cStackTop += *(uint32 *) &dt;
                        break;
                    case i64:
                        *(int64 *) clStackTop->cStackTop += *(int64 *) &dt;
                        break;
                    case u64:
                        *(uint64 *) clStackTop->cStackTop += *(uint64 *) &dt;
                        break;
                    case f32:
                        *(float32 *) clStackTop->cStackTop += *(float32 *) &dt;
                        break;
                    case f64:
                        *(float64 *) clStackTop->cStackTop += *(float64 *) &dt;
                        break;
                }
                break;
            }
            case sub: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *) clStackTop->cStackTop -= *(int8 *) &dt;
                        break;
                    case u8:
                        *(uint8 *) clStackTop->cStackTop -= *(uint8 *) &dt;
                        break;
                    case i16:
                        *(int16 *) clStackTop->cStackTop -= *(int16 *) &dt;
                        break;
                    case u16:
                        *(uint16 *) clStackTop->cStackTop -= *(uint16 *) &dt;
                        break;
                    case i32:
                        *(int32 *) clStackTop->cStackTop -= *(int32 *) &dt;
                        break;
                    case u32:
                        *(uint32 *) clStackTop->cStackTop -= *(uint32 *) &dt;
                        break;
                    case i64:
                        *(int64 *) clStackTop->cStackTop -= *(int64 *) &dt;
                        break;
                    case u64:
                        *(uint64 *) clStackTop->cStackTop -= *(uint64 *) &dt;
                        break;
                    case f32:
                        *(float32 *) clStackTop->cStackTop -= *(float32 *) &dt;
                        break;
                    case f64:
                        *(float64 *) clStackTop->cStackTop -= *(float64 *) &dt;
                        break;
                }
                break;
            }
            case mul: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *) clStackTop->cStackTop *= *(int8 *) &dt;
                        break;
                    case u8:
                        *(uint8 *) clStackTop->cStackTop *= *(uint8 *) &dt;
                        break;
                    case i16:
                        *(int16 *) clStackTop->cStackTop *= *(int16 *) &dt;
                        break;
                    case u16:
                        *(uint16 *) clStackTop->cStackTop *= *(uint16 *) &dt;
                        break;
                    case i32:
                        *(int32 *) clStackTop->cStackTop *= *(int32 *) &dt;
                        break;
                    case u32:
                        *(uint32 *) clStackTop->cStackTop *= *(uint32 *) &dt;
                        break;
                    case i64:
                        *(int64 *) clStackTop->cStackTop *= *(int64 *) &dt;
                        break;
                    case u64:
                        *(uint64 *) clStackTop->cStackTop *= *(uint64 *) &dt;
                        break;
                    case f32:
                        *(float32 *) clStackTop->cStackTop *= *(float32 *) &dt;
                        break;
                    case f64:
                        *(float64 *) clStackTop->cStackTop *= *(float64 *) &dt;
                        break;
                }
                break;
            }
            case _div: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case i8:
                        *(int8 *) clStackTop->cStackTop /= *(int8 *) &dt;
                        break;
                    case u8:
                        *(uint8 *) clStackTop->cStackTop /= *(uint8 *) &dt;
                        break;
                    case i16:
                        *(int16 *) clStackTop->cStackTop /= *(int16 *) &dt;
                        break;
                    case u16:
                        *(uint16 *) clStackTop->cStackTop /= *(uint16 *) &dt;
                        break;
                    case i32:
                        *(int32 *) clStackTop->cStackTop /= *(int32 *) &dt;
                        break;
                    case u32:
                        *(uint32 *) clStackTop->cStackTop /= *(uint32 *) &dt;
                        break;
                    case i64:
                        *(int64 *) clStackTop->cStackTop /= *(int64 *) &dt;
                        break;
                    case u64:
                        *(uint64 *) clStackTop->cStackTop /= *(uint64 *) &dt;
                        break;
                    case f32:
                        *(float32 *) clStackTop->cStackTop /= *(float32 *) &dt;
                        break;
                    case f64:
                        *(float64 *) clStackTop->cStackTop /= *(float64 *) &dt;
                        break;
                }
                break;
            }
#pragma endregion

#pragma region compare operator (< > <= >= == !=)
            case gt: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop > *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop > *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop > *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop > *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop > *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop > *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop > *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop > *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop > *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop > *(float64 *) &dt2);
                        break;
                }
                break;
            }
            case ge: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop >= *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop >= *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop >= *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop >= *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop >= *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop >= *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop >= *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop >= *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop >= *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop >= *(float64 *) &dt2);
                        break;
                }
                break;
            }
            case ls: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop < *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop < *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop < *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop < *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop < *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop < *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop < *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop < *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop < *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop < *(float64 *) &dt2);
                        break;
                }
                break;
            }
            case le: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop <= *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop <= *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop <= *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop <= *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop <= *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop <= *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop <= *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop <= *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop <= *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop <= *(float64 *) &dt2);
                        break;
                }
                break;
            }
            case eq: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop == *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop == *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop == *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop == *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop == *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop == *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop == *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop == *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop == *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop == *(float64 *) &dt2);
                        break;
                }
                break;
            }
            case ne: {
                getTrueValue(vlmdf2, dtmdf1);
                uint64 dt2 = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, dtmdf1);
                switch (dtmdf1) {
                    case u8:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop != *(uint8 *) &dt2);
                        break;
                    case i8:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop != *(int8 *) &dt2);
                        break;
                    case u16:
                        *(clStackTop->cStackTop) = (uint64) (*(uint16 *) clStackTop->cStackTop != *(uint16 *) &dt2);
                        break;
                    case i16:
                        *(clStackTop->cStackTop) = (uint64) (*(int16 *) clStackTop->cStackTop != *(int16 *) &dt2);
                        break;
                    case u32:
                        *(clStackTop->cStackTop) = (uint64) (*(uint32 *) clStackTop->cStackTop != *(uint32 *) &dt2);
                        break;
                    case i32:
                        *(clStackTop->cStackTop) = (uint64) (*(int32 *) clStackTop->cStackTop != *(int32 *) &dt2);
                        break;
                    case u64:
                        *(clStackTop->cStackTop) = (uint64) (*(uint8 *) clStackTop->cStackTop != *(uint64 *) &dt2);
                        break;
                    case i64:
                        *(clStackTop->cStackTop) = (uint64) (*(int8 *) clStackTop->cStackTop != *(int64 *) &dt2);
                        break;
                    case f32:
                        *(clStackTop->cStackTop) = (uint64) (*(float32 *) clStackTop->cStackTop != *(float32 *) &dt2);
                        break;
                    case f64:
                        *(clStackTop->cStackTop) = (uint64) (*(float64 *) clStackTop->cStackTop != *(float64 *) &dt2);
                        break;
                }
                break;
            }
#pragma endregion

#pragma region inc and dec
            case sinc: {
                switch (dtmdf1) {
                    case u8: {
                        *clStackTop->cStackTop = (++(*(uint8 *) *clStackTop->cStackTop) - 1) & ((1ull << 8) - 1);
                        break;
                    }
                    case i8: {
                        *clStackTop->cStackTop = (++(*(int8 *) *clStackTop->cStackTop) - 1) & ((1ull << 8) - 1);
                        break;
                    }
                    case u16: {
                        *clStackTop->cStackTop = (++(*(uint16 *) *clStackTop->cStackTop) - 1) & ((1ull << 16) - 1);
                        break;
                    }
                    case i16: {
                        *clStackTop->cStackTop = (++(*(int16 *) *clStackTop->cStackTop) - 1) & ((1ull << 16) - 1);
                        break;
                    }
                    case u32: {
                        *clStackTop->cStackTop = (++(*(uint32 *) *clStackTop->cStackTop) - 1) & ((1ull << 32) - 1);
                        break;
                    }
                    case i32: {
                        *clStackTop->cStackTop = (++(*(int32 *) *clStackTop->cStackTop) - 1) & ((1ull << 32) - 1);
                        break;
                    }
                    case u64: {
                        *clStackTop->cStackTop = (++(*(uint64 *) *clStackTop->cStackTop) - 1);
                        break;
                    }
                    case i64: {
                        *clStackTop->cStackTop = (++(*(int64 *) *clStackTop->cStackTop) - 1);
                        break;
                    }
                }
                if (vlmdf1 == MemberRef) {
                    Object *obj = *(clStackTop->oStackTop--);
                    obj->refCount--, obj->rootRefCount--;
                    if (!obj->refCount) refGC(obj);
                }
                break;
            }
            case pinc:
                switch (dtmdf1) {
                    case u8: {
                        *clStackTop->cStackTop = (++(*(uint8 *) *clStackTop->cStackTop)) & ((1ull << 8) - 1);
                        break;
                    }
                    case i8: {
                        *clStackTop->cStackTop = (++(*(int8 *) *clStackTop->cStackTop)) & ((1ull << 8) - 1);
                        break;
                    }
                    case u16: {
                        *clStackTop->cStackTop = (++(*(uint16 *) *clStackTop->cStackTop)) & ((1ull << 16) - 1);
                        break;
                    }
                    case i16: {
                        *clStackTop->cStackTop = (++(*(int16 *) *clStackTop->cStackTop)) & ((1ull << 16) - 1);
                        break;
                    }
                    case u32: {
                        *clStackTop->cStackTop = (++(*(uint32 *) *clStackTop->cStackTop)) & ((1ull << 32) - 1);
                        break;
                    }
                    case i32: {
                        *clStackTop->cStackTop = (++(*(int32 *) *clStackTop->cStackTop)) & ((1ull << 32) - 1);
                        break;
                    }
                    case u64: {
                        *clStackTop->cStackTop = (++(*(uint64 *) *clStackTop->cStackTop));
                        break;
                    }
                    case i64: {
                        *clStackTop->cStackTop = (++(*(int64 *) *clStackTop->cStackTop));
                        break;
                    }
                }
                if (vlmdf1 == MemberRef) {
                    Object *obj = *(clStackTop->oStackTop--);
                    obj->refCount--, obj->rootRefCount--;
                    if (!obj->refCount) refGC(obj);
                }
                break;
            case sdec: {
                switch (dtmdf1) {
                    case u8: {
                        *clStackTop->cStackTop = (--(*(uint8 *) *clStackTop->cStackTop) + 1) & ((1ull << 8) - 1);
                        break;
                    }
                    case i8: {
                        *clStackTop->cStackTop = (--(*(int8 *) *clStackTop->cStackTop) + 1) & ((1ull << 8) - 1);
                        break;
                    }
                    case u16: {
                        *clStackTop->cStackTop = (--(*(uint16 *) *clStackTop->cStackTop) + 1) & ((1ull << 16) - 1);
                        break;
                    }
                    case i16: {
                        *clStackTop->cStackTop = (--(*(int16 *) *clStackTop->cStackTop) + 1) & ((1ull << 16) - 1);
                        break;
                    }
                    case u32: {
                        *clStackTop->cStackTop = (--(*(uint32 *) *clStackTop->cStackTop) + 1) & ((1ull << 32) - 1);
                        break;
                    }
                    case i32: {
                        *clStackTop->cStackTop = (--(*(int32 *) *clStackTop->cStackTop) + 1) & ((1ull << 32) - 1);
                        break;
                    }
                    case u64: {
                        *clStackTop->cStackTop = (--(*(uint64 *) *clStackTop->cStackTop) + 1);
                        break;
                    }
                    case i64: {
                        *clStackTop->cStackTop = (--(*(int64 *) *clStackTop->cStackTop) + 1);
                        break;
                    }
                }
                if (vlmdf1 == MemberRef) {
                    Object *obj = *(clStackTop->oStackTop--);
                    obj->refCount--, obj->rootRefCount--;
                    if (!obj->refCount) refGC(obj);
                }
                break;
            }
            case pdec: {
                switch (dtmdf1) {
                    case u8: {
                        *clStackTop->cStackTop = (--(*(uint8 *) *clStackTop->cStackTop)) & ((1ull << 8) - 1);
                        break;
                    }
                    case i8: {
                        *clStackTop->cStackTop = (--(*(int8 *) *clStackTop->cStackTop)) & ((1ull << 8) - 1);
                        break;
                    }
                    case u16: {
                        *clStackTop->cStackTop = (--(*(uint16 *) *clStackTop->cStackTop)) & ((1ull << 16) - 1);
                        break;
                    }
                    case i16: {
                        *clStackTop->cStackTop = (--(*(int16 *) *clStackTop->cStackTop)) & ((1ull << 16) - 1);
                        break;
                    }
                    case u32: {
                        *clStackTop->cStackTop = (--(*(uint32 *) *clStackTop->cStackTop)) & ((1ull << 32) - 1);
                        break;
                    }
                    case i32: {
                        *clStackTop->cStackTop = (--(*(int32 *) *clStackTop->cStackTop)) & ((1ull << 32) - 1);
                        break;
                    }
                    case u64: {
                        *clStackTop->cStackTop = (--(*(uint64 *) *clStackTop->cStackTop));
                        break;
                    }
                    case i64: {
                        *clStackTop->cStackTop = (--(*(int64 *) *clStackTop->cStackTop));
                        break;
                    }
                }
                if (vlmdf1 == MemberRef) {
                    Object *obj = *(clStackTop->oStackTop--);
                    obj->refCount--, obj->rootRefCount--;
                    if (!obj->refCount) refGC(obj);
                }
                break;
            }
#pragma endregion

            case push:
                *(++clStackTop->cStackTop) = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                break;
            case pop: {
                getTrueValue(vlmdf1, dtmdf1);
                if (dtmdf1 == o) {
                    Object *obj = (Object *) *clStackTop->cStackTop;
                    if (obj) {
                        obj->refCount--, obj->rootRefCount--;
                        if (!obj->refCount) refGC(obj);
                    }
                }
                clStackTop->cStackTop--;
                break;
            }
            case pvar:
                *(++clStackTop->cStackTop) = (uint64) &clStackTop->var[*(uint64 *) &curRBlock->vcode[clStackTop->offset]];
                if (dtmdf1 == o) {
                    Object *obj = (Object *) *(uint64 *) *clStackTop->cStackTop;
                    if (obj) obj->rootRefCount++, obj->refCount++;
                }
                clStackTop->offset += sizeof(uint64);
                break;
            case pglo: {
                uint64 id = *(uint64 *) &curRBlock->vcode[clStackTop->offset], offset = id & ((1ull << 48) - 1);
                uint32 blkid = getRelyBlkId((id >> 48) & ((1ull << 16) - 1));
                *(++clStackTop->cStackTop) = (uint64) (rBlocks[blkid].globalMemory + offset);
                if (dtmdf1 == o) {
                    Object *obj = (Object *) *(uint64 *) *clStackTop->cStackTop;
                    if (obj) obj->rootRefCount++, obj->refCount++;
                }
                clStackTop->offset += sizeof(uint64);
                break;
            }

            case _new: {
                uint64 tid = *(uint64 *) &curRBlock->vcode[clStackTop->offset], offset = tid & ((1ull << 48) - 1);
                clStackTop->offset += sizeof(uint64);
                const uint32 blkid = getRelyBlkId((tid >> 48) & ((1ull << 16) - 1));

                // get the information from the data template
                uint8 *dtTemplate = rBlocks[blkid].dataTemplate + offset;
                uint64 size = ((ClassTypeData *) *(uint64 *) dtTemplate)->size;

                Object *obj = newObject(size);
                obj->rootRefCount = obj->refCount = 1;
                obj->typeData = (ClassTypeData *) dtTemplate;
                // copy the dtTemplate into the data block of obj
                memcpy(obj->data, dtTemplate, size);

                *(++clStackTop->cStackTop) = (uint64) obj;
                break;
            }
            case arrnew: {
                uint64 dimc = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);

                // calculate the size
                for (int i = dimc; i >= 0; i--) tmpData[i] = *(clStackTop->cStackTop--);
                for (int i = 1; i <= dimc; i++) tmpData[i] *= tmpData[i - 1];

                Object *obj = newObject(dimc * sizeof(uint64) + tmpData[dimc]);
                obj->refCount = obj->rootRefCount = 1;
                for (int i = 0; i < dimc; i++) ((uint64 *) obj->data)[i] = tmpData[i];

                *(++clStackTop->cStackTop) = (uint64) obj;
                break;
            }

            case mem: {
                uint64 offset = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);

                // put the object into OStack
                getTrueValue(vlmdf1, o);
                Object *obj = *(++clStackTop->oStackTop) = (Object *) *clStackTop->cStackTop;

                *clStackTop->cStackTop = (uint64) (obj->data + offset);
                // this field is an object
                if (dtmdf1 == o) {
                    // set the flag of this object
                    uint64 pos = offset / 8 / 64;
                    obj->flag[pos] |= (1ull << (offset - pos * 64));
                    // update the reference count of this child
                    Object *child = (Object *) *(uint64 *) (obj->data + offset);
                    if (child) child->rootRefCount++, child->refCount++;
                }
                break;
            }
            case arrmem: {
                uint64 dimc = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);

                for (int i = dimc - 1; i >= 0; i--) tmpData[i] = *(clStackTop->cStackTop--);
                getTrueValue(vlmdf1, o);
                Object *arr = (Object *) *clStackTop->cStackTop;
                uint64 offset = dimc * sizeof(uint64);
                for (int i = 0; i < dimc; i++) offset += ((uint64 *) arr->data)[i] * tmpData[i];

                // push the array object into oStack
                *(++clStackTop->oStackTop) = arr;
                *clStackTop->cStackTop = (uint64) (arr->data + dimc * sizeof(uint64) + offset);

                // this element is an object
                if (dtmdf1 == o) {
                    // set the flag of this array
                    uint64 pos = offset / 8 / 64;
                    arr->flag[pos] |= (1ull << (offset - pos * 64));
                    // update the reference count of this child
                    Object *child = (Object *) *(uint64 *) (arr->data + offset);
                    if (child) child->rootRefCount++, child->refCount++;
                }
                break;
            }

#pragma region jump operator
            case jz: {
                uint64 offset = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                getTrueValue(vlmdf1, u64);
                if (*(clStackTop->cStackTop--)) clStackTop->offset += sizeof(uint64);
                else clStackTop->offset = offset;
                break;
            }
            case jp: {
                uint64 offset = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                getTrueValue(vlmdf1, u64);
                if (!*(clStackTop->cStackTop--)) clStackTop->offset += sizeof(uint64);
                else clStackTop->offset = offset;
                break;
            }
            case jmp: {
                clStackTop->offset = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                break;
            }
#pragma endregion

#pragma region function call operator
            case call: {
                uint64 addr = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                uint32 blkId = getRelyBlkId(addr >> 48 & ((1ull << 16) - 1));
                uint64 offset = addr & ((1ull << 48) - 1);
                callFunc(blkId, offset);
                break;
            }
            case ret: {
                retFunc();
                break;
            }
            case vret: {
                retFuncV();
                break;
            }
            case setlocal: {
                uint64 number = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                clStackTop->var = mallocArray(uint64, number);
                memset(clStackTop->var, 0, sizeof(uint64) * number);
                break;
            }
            case setarg: {
                uint64 number = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                for (int i = number - 1; i >= 0; i--) argData[i] = *(clStackTop->cStackTop--);
                break;
            }
            case getarg: {
                uint64 number = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                for (int i = number - 1; i >= 0; i--) clStackTop->var[i] = argData[i];
                break;
            }
            case sys: {
                uint64 id = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                switch (id) {
                    case 0x0:
                        printf("%d", *(int32 *) clStackTop->cStackTop);
                        clStackTop->cStackTop--;
                        break;
                    case 0x1: {
                        *(++clStackTop->cStackTop) = 0;
                        scanf("%d", (uint32 *) clStackTop->cStackTop);
                        break;
                    }
                }
                break;
            }
            case setgtbl : {
                uint64 size = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                for (int i = size - 1; i >= 0; i--)
                    (clStackTop + 1)->genericTable[i] = *(clStackTop->cStackTop--);
                break;
            }
            case getgtbl : {
                uint64 id = *(uint64 *) &curRBlock->vcode[clStackTop->offset];
                clStackTop->offset += sizeof(uint64);
                *(++clStackTop->cStackTop) = clStackTop->genericTable[id];
                break;
            }
            default:
                printf("Unknown command : %x\n", vcode);
                return;
                break;
#pragma endregion
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

    clock_t st = clock();
    mainLoop();
    clock_t ed = clock();
    printf("\nexit, cost %f(s)\n", 1.0 * (1.0 * ed - 1.0 * st) / CLOCKS_PER_SEC);
    return 0;
}