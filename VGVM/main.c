#include "tmpl.h"

int32 data[10];

RuntimeBlock *testBlock;

int main() {
    initGC();

    testBlock = loadRuntimeBlock("./main.vobj");
    clock_t st = clock();
    launch(testBlock);
    printf("%.3lf\n", (clock() - st) * 1.0 / CLOCKS_PER_SEC);
    return 0;
}