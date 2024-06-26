#include "syscall.h"
#include "vgvm.h"

uint64 syscallArgCnt[] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1 };
void *syscall[] = {VMputchar, VMgetchar, printInt, inputInt, printInt64, inputInt64, printUInt64, inputUInt64, getTime, NULL, printFloat32 };

void VMputchar(char ch) { putchar(ch); }
char VMgetchar() { return getchar(); }
void printInt(int32 data) {
    AlignRsp
    printf("%d", data);
    cancelAlignRsp
}
int32 inputInt() { 
    AlignRsp
    int32 data; scanf("%d", &data); return data; 
    cancelAlignRsp
}
void printInt64(int64 data) {
    AlignRsp
    printf("%lld", data);
    cancelAlignRsp
}
int64 inputInt64() { 
    AlignRsp
    int64 data; scanf("%lld", &data); return data;
    cancelAlignRsp
}
void printUInt64(uint64 data) {
    AlignRsp
    printf("%llu", data);
    cancelAlignRsp
}
uint64 inputUInt64() {
    AlignRsp
    uint64 data; scanf("%llu", &data); return data;
    cancelAlignRsp 
}
uint64 getTime() { AlignRsp return time(NULL); cancelAlignRsp }

void printFloat32(uint64 data) {
    AlignRsp
    printf("%.10f", *(float32 *)&data); 
    cancelAlignRsp 
}