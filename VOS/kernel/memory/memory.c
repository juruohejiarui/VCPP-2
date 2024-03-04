#include "../includes/memory.h"

GlobalMemoryDescriptor memManageStruct = {{0}, 0};

void initMemory() {
    E820 *p = (E820 *)0xffff800000007e00;
}