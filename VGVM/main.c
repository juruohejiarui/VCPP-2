#include "tmpl.h"

int32 data[10];

RuntimeBlock *testBlock;

int main() {
    freopen("input.in", "r", stdin);
    initGC();
    uint64 oldRsp = 0, newRsp = 0;
    testBlock = loadRuntimeBlock("./main.vobj");
    clock_t st = clock();
    launch(testBlock);
    genGC(1);
    freeRuntimeblock();
    
    #ifndef NDEBUG
    printf("%.3lf\n", (clock() - st) * 1.0 / CLOCKS_PER_SEC);
    printf("remains %llu + %llu\n", getGenSize(0), getGenSize(1));
    #endif
    printf("exiting vgvm...\n");
    return 0;
}