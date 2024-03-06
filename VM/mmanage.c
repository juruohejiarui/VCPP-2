#include "mmanage.h"

uint64 genSize[2];
uint64 checkTick;
const uint64 limitGenSize[2] = {1 << 26, 1 << 30}, limitCheckTick = 2;

int checkGC() {
    ++checkTick;
    return (genSize[0] > limitGenSize[0] || genSize[1] > limitGenSize[1]) && checkTick >= limitCheckTick;
}

uint64 getGenSize(int genId) { return genSize[genId]; }

ListElement objListStart[2], objListEnd[2], freeListStart, freeListEnd;

void initGC() {
    List_init(&objListStart[0], &objListEnd[0]), List_init(&objListStart[1], &objListEnd[1]);
    List_init(&freeListStart, &freeListEnd);
}

Object *newObject(uint64 size) {
    AlignRsp
    Object *obj = NULL;

    // get an object from free list
    if (freeListStart.next != &freeListEnd) {
        obj = freeListStart.next->content;
        List_remove(freeListStart.next);
    } else {
        ListElement *lsele = mallocObj(ListElement);
        obj = mallocObj(Object);
        obj->belong = lsele, lsele->content = obj;
    }
    #ifndef NDEBUG
    printf("GC Log:   object Address : %p (%#018llx = %.6lfMB(datas) + %#018llx = %.6lfMB(flags))\n", obj,
        size,
        size * 1.0 / 1024 / 1024, 
        ((size / 8) + 63) / 64,
        (((size / 8) + 63) / 64) * sizeof(uint64) * 1.0 / 1024 / 1024);
    #endif
    obj->dataSize = size, obj->flagSize = ((size / 8) + 63) / 64;
    obj->data = mallocArray(uint8, size);
    obj->flag = mallocArray(uint64, obj->flagSize);
    memset(obj->data, 0, sizeof(uint8) * size);
    memset(obj->flag, 0, sizeof(uint64) * obj->flagSize); 
    obj->genId = 0, obj->state = ObjectState_Active;

    List_insert(&objListEnd[0], obj->belong);

    obj->refCount = obj->rootRefCount = 1, obj->crossRefCount = 0;
    genSize[0] += size;
    if (checkGC()) genGC(0);
    cancelAlignRsp
    return obj;
}

void freeObj(Object *obj) {
    AlignRsp
    #ifndef NDEBUG
    printf("GC Log:   free obj : %p in gen %d\n", obj, obj->genId);
    #endif
    genSize[obj->genId] -= obj->dataSize;
    free(obj->data), free(obj->flag);
    obj->dataSize = obj->flagSize = 0;
    obj->state = ObjectState_Free;
    List_remove(obj->belong), List_insert(&freeListEnd, obj->belong);
    cancelAlignRsp
}

void refGC(Object *obj) {
    if (obj->refCount > 0 || obj->state == ObjectState_Free) return ;
    obj->state = ObjectState_Free;
    for (uint32 i = 0; i < obj->flagSize; i++) if (obj->flag[i]) {
        for (int j = 0; j < 64 && (j + i * 64) * 8 + 8 <= obj->dataSize; j++) if (obj->flag[i] & (1ull << j)) {
            Object *ref = *(uint64*)&obj->data[(i * 64 + j) * 8];
            if (ref == NULL || --ref->refCount) continue;
            refGC(ref);
        }
    }
    freeObj(obj);
}

void scanObj(Object *obj, uint32 mxGen) {
    if (obj->state == ObjectState_Active) return ;
    obj->state = ObjectState_Active;
    for (uint32 i = 0; i < obj->flagSize; i++) if (obj->flag[i]) {
        for (int j = 0; j < 64 && (j + i * 64) * 8 + 8 <= obj->dataSize; i++) if (obj->flag[i] & (1ull << j)) {
            Object *ref = *(uint64*) &obj->data[(i * 64 + j) * 8];
            if (ref == NULL || ref->genId > mxGen || ref->state == ObjectState_Active) continue;
            scanObj(ref, mxGen);
        }
    }
}

void genGC(int isFinal) {
    for (ListElement *ele = objListStart[0].next; ele != &objListEnd[0]; ele = ele->next) ((Object *)ele->content)->state = ObjectState_Waiting;
    for (ListElement *ele = objListStart[0].next; ele != &objListEnd[0]; ele = ele->next) {
        Object *obj = (Object *)ele->content;
        if (obj->rootRefCount || obj->crossRefCount) scanObj(obj, 0);
    }
    for (ListElement *ele = objListStart[0].next, *nxt; ele != &objListEnd[0]; ele = nxt) {
        Object *obj = (Object *)ele->content;
        nxt = ele->next;
        if (obj->state == ObjectState_Waiting) freeObj(obj);
    }
    // move all the objects in generation 0 into generation 1
    for (ListElement *ele = objListStart[0].next, *nxt; ele != &objListEnd[0]; ele = nxt) {
        Object *obj = (Object *)ele->content;
        nxt = ele->next;
        obj->genId = 1, obj->crossRefCount = 0;
        List_remove(ele), List_insert(&objListEnd[1], ele);
        genSize[1] += obj->dataSize, genSize[0] -= obj->dataSize;
    }
    if (genSize[1] > limitGenSize[1] || isFinal) {
        for (ListElement *ele = objListStart[1].next; ele != &objListEnd[1]; ele = ele->next) ((Object *)ele->content)->state = ObjectState_Waiting;
        for (ListElement *ele = objListStart[1].next; ele != &objListEnd[1]; ele = ele->next) {
            Object *obj = (Object *)ele->content;
            if (obj->rootRefCount || obj->crossRefCount) scanObj(obj, 1);
        }
        for (ListElement *ele = objListStart[1].next, *nxt; ele != &objListEnd[1]; ele = nxt) {
            Object *obj = (Object *)ele->content;
            nxt = ele->next;
            if (obj->state == ObjectState_Waiting) freeObj(obj);
        }
    }
    #ifndef NDEBUG
    if (isFinal) {
        printf("remains : \ngen 1: ");
        for (ListElement *ele = objListStart[1].next; ele != &objListEnd[1]; ele = ele->next) {
            Object *obj = (Object *)ele->content;
            printf("%p ref = %lld + %lld\n", obj, obj->refCount, obj->rootRefCount);
        } 
        printf("gen 0: ");
        for (ListElement *ele = objListStart[0].next; ele != &objListEnd[0]; ele = ele->next) {
            Object *obj = (Object *)ele->content;
            printf("%p ref = %lld + %lld\n", obj, obj->refCount, obj->rootRefCount);
        } 
        putchar('\n');
    }
    #endif
    checkTick = 0;
}
