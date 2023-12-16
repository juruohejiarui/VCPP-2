#include "vm.h"

#define BLK_ERROR_ID (1u << 31)

RuntimeBlock rBlocks[1024];
int rBlockCount = 0;
uint32 loadRuntimeBlock(const char *path) {
    FILE *fPtr = fopen(path, "rb");
    int blkid = rBlockCount++;

}