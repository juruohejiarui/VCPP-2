#include "tools.h"

void Debug_printJITCode(uint8 *instBlk, uint64 size) {
    AlignRsp
    FILE *fPtr = fopen("test.bin", "wb");
    fwrite(instBlk, sizeof(uint8), size, fPtr);
    fclose(fPtr);
    system("objdump -D -b binary -m i386:x86-64 test.bin");
    cancelAlignRsp
}

void Debug_saveJITCode(uint8 *instBlk, uint64 size, char *logPath) {
    AlignRsp
    FILE *fPtr = fopen("test.bin", "wb");
    fwrite(instBlk, sizeof(uint8), size, fPtr);
    fclose(fPtr);
    static char buf[10224];
    sprintf(buf, "objdump -D -b binary -m i386:x86-64 test.bin > %s\0", logPath);
    system(buf);
    cancelAlignRsp
}
