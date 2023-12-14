#include "mmanage.h"

uint64 genSize[2];
uint64 checkTick;
const uint64 limitGenSize[2] = {1 << 26, 1 << 30}, limitCheckTick = 1 << 18;

int checkGC() {
    return (genSize[0] > limitGenSize[0] || genSize[1] > limitGenSize[1]) && checkTick >= limitCheckTick;
}

ListElement objListStart[2], objListEnd[2], freeListStart, freeListEnd;

void initGC() {
    List_init(&objListStart[0], &objListEnd[0]), List_init(&objListStart[1], &objListEnd[1]);
    List_init(&freeListStart, &freeListEnd);
}