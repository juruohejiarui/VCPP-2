#include "vgvm.h"
#include "tmpl.h"
#include "../VM/mmanage.h"

uint64 paramData[16], gtable[16], gtablesz[16] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 8}, VMflag;

RuntimeBlock rBlks[1 << 16];
uint32 rBlkCnt = 0;

TrieNode vobjPathTrieRoot;

int launch(RuntimeBlock *rBlk) {
    return ((uint64 (*)(RuntimeBlock *))(rBlk->entryList[rBlk->mainOffset]))(rBlk);
}

void genInstBlk(RuntimeBlock *rblk) {
    setTargetBlock(rblk);
    uint64 offset = 0;
    while (offset < rblk->vinstListSize) {
        uint32 vcode = *(uint32 *) &rblk->vinstList[offset], tcmd = vcode & ((1 << 16) - 1);
        uint16  dtmdf1 = (vcode >> 16) & 15,
                dtmdf2 = (vcode >> 20) & 15;
        uint64 arg1 = 0, arg2 = 0, tmpOffset = offset;
        offset += sizeof(uint32);
        if (argCnt[tcmd] >= 1)
            arg1 = *(uint64 *)&rblk->vinstList[offset],
            offset += sizeof(uint64);
        if (argCnt[tcmd] >= 2)
            arg2 = *(uint64 *)&rblk->vinstList[offset],
            offset += sizeof(uint64);
        addVInst(tmpOffset, tcmd, dtmdf1, dtmdf2, arg1, arg2);
    }
    setInstBlk();
}

RuntimeBlock *loadRuntimeBlock(const char *vobjPath) {
    AlignRsp
    #ifndef NDEBUG
    printf("VGVM Log: loading rblock from file %s\n", vobjPath);
    #endif
    FILE *fPtr = fopen(vobjPath, "rb");

    uint32 blkid = ++rBlkCnt;
    RuntimeBlock *rblk = &rBlks[blkid];
    rblk->id = blkid;

    uint8 type; readData(fPtr, &type, uint8);
    
    readData(fPtr, &rblk->relyCnt, uint64);
    rblk->relyBlk = mallocArray(RuntimeBlock *, rblk->relyCnt + 1);
    rblk->relyPath = mallocArray(char *, rblk->relyCnt + 1);
    memset(rblk->relyBlk, 0, sizeof(RuntimeBlock *) * (rblk->relyCnt + 1));
    for (int i = 1; i <= rblk->relyCnt; i++)
        rblk->relyPath[i] = readString(fPtr);
    rblk->relyBlk[0] = rblk;
    ignoreString(fPtr);

    rblk->tdRoot = loadTypeData(fPtr);
    rblk->dataTmpl = generateDataTmpl(rblk->tdRoot);

    readData(fPtr, &rblk->mainOffset, uint64);

    readData(fPtr, &rblk->strCnt, uint64);
    rblk->strList = mallocArray(Object *, rblk->strCnt);
    for (int i = 0; i < rblk->strCnt; i++) {
        char *str = readString(fPtr);
        size_t strl = strlen(str);
        Object *strObj = newObject(strl + 1 + sizeof(uint64));
        strObj->refCount = strObj->rootRefCount = 1;
        *(uint64 *)strObj->data = sizeof(int8);
        memcpy(strObj->data + sizeof(uint64), str, strl + 1);
        free(str);
        rblk->strList[i] = strObj;
    }
    uint64 vcodeSize;
    // read global memory size
    readData(fPtr, &rblk->gloMemSize, uint64);
    rblk->gloMem = mallocArray(uint8, rblk->gloMemSize);
    memset(rblk->gloMem, 0, sizeof(uint8) * rblk->gloMemSize);

    readData(fPtr, &rblk->vinstListSize, uint64);
    rblk->vinstList = mallocArray(uint8, rblk->vinstListSize);
    readArray(fPtr, rblk->vinstList, uint8, rblk->vinstListSize);

    Trie_insert(&vobjPathTrieRoot, vobjPath, rblk);

    genInstBlk(rblk);

    cancelAlignRsp
    return rblk;
}

uint64 stkData[105];
char *regName[] = {"RAX", "RCX", "RDX", "RBX", "RSI", "RDI", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

void pauseVM(uint64 rbp, uint64 rsp) {
    printf("VGVM Log: pause\n");
    uint64 i;
    for (i = 0; rbp - (i + 1) * sizeof(uint64) >= rsp; i++) stkData[i] = *(uint64 *)(rbp - (i + 1) * sizeof(uint64));
    printf("%%rbp = %p, %%rsp = %p\n", rbp, rsp + sizeof(uint64) * 14);
    printf("runtimeBlock Address: %p\n", stkData[0]);
    RuntimeBlock *rblk = (RuntimeBlock *)stkData[0];
    for (int i = 0; i * sizeof(uint64) < rblk->gloMemSize; i++) printf("%#018llx%c", ((uint64 *)rblk->gloMem)[i], ((((i + 1)) % 4 == 0 ? '\n' : ' ')));
    if (rblk->gloMemSize / sizeof(uint64) % 4 != 0) putchar('\n');
    printf("gtable Data         : %#018llx\n", stkData[1]);
    for (int j = 0; j < 5; j++) printf("clStkData[%2d]: %#018llx\n", 2 + j, stkData[2 + j]);
    for (int j = 0; j + 5 + 2 < i - 14; j++) printf("Var[%d]: %#018llx\n", j, stkData[7 + j]);
    for (int j = 0; j < 14; j++) printf("%3s : %#018llx%c", regName[j], stkData[i - 14 + j], (j + 1) % 4 != 0 ? ' ' : '\n');
    putchar('\n');
    // printf("%s--------------------\n", (i % 4 != 0 ? "\n" : ""));
    for (int i = 0; i < 16; i++) printf("%#018llx%c", paramData[i], ((i + 1) % 4 == 0 ? '\n' : ' '));
}

uint64 callFunc(uint64 id) {
    uint64 offset = id & ((1ull << 48) - 1);
    RuntimeBlock *rBlk = &rBlks[id >> 48];
    return ((uint64 (*)(RuntimeBlock *))(rBlk->entryList[offset]))(rBlk);
}

void disconnectMember(Object *obj, Object *mem) {
    // printf("disconnect %p from %p\n", mem, obj);
    if (mem == NULL) return ;
    mem->refCount--;
    if (obj->genId > mem->genId) mem->crossRefCount--;
    if (!mem->refCount) refGC(mem);
}
void connectMember(Object *obj, Object *mem, uint64 offset) {
    // printf("connect %p to %p\n", mem, obj);
    if (mem == NULL) return ;
    mem->rootRefCount--;
    obj->flag[offset / 8 / 64] |= (1ull << (offset / 8 % 64));
    if (obj->genId > mem->genId) mem->crossRefCount++;
}

void *getGloAddr(RuntimeBlock *curBlk, uint64 id) {
    uint16 blkId = id >> 48, offset = id & ((1ull << 48) - 1), res;
    RuntimeBlock *rBlk = NULL;
    if (curBlk->relyBlk[blkId] == NULL) curBlk->relyBlk[blkId] = loadRuntimeBlock(curBlk->relyPath[blkId]);
    rBlk = curBlk->relyBlk[blkId];
    return (void *)(rBlk->gloMem + offset);
}

uint64 getRelyId(RuntimeBlock *curBlk, uint64 id) {
    if (curBlk->relyBlk[id] == NULL) curBlk->relyBlk[id] = loadRuntimeBlock(curBlk->relyPath[id]);
    return curBlk->relyBlk[id]->id;
}

RuntimeBlock *getRuntimeBlock(uint64 blkId) {
    if (blkId > rBlkCnt) return NULL;
    return &rBlks[blkId];
}
