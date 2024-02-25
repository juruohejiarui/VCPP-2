#include "syscall.h"
#include "vgvm.h"

uint64 syscallArgCnt[] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1 };
void *syscall[] = {VMputchar, VMgetchar, printInt, inputInt, printInt64, inputInt64, printUInt64, inputUInt64, getTime, NULL, printFloat32 };

void VMputchar(char ch) { putchar(ch); }
char VMgetchar() { return getchar(); }
void printInt(int32 data) {
    #ifdef GCC_HIGH
AlignRsp
    #endif
    printf("%d", data); }
int32 inputInt() { 
    #ifdef GCC_HIGH
AlignRsp
    #endif
    int32 data; scanf("%d", &data); return data; 
}
void printInt64(int64 data) {
    #ifdef GCC_HIGH
    AlignRsp
    #endif
printf("%lld", data);
}
int64 inputInt64() { int64 data; scanf("%lld", &data); return data; }
void printUInt64(uint64 data) {
    #ifdef GCC_HIGH
    AlignRsp
    #endif
    printf("%llu", data); 
    }
uint64 inputUInt64() { uint64 data;
    #ifdef GCC_HIGH
    AlignRsp
    #endif
    scanf("%llu", &data); return data; 
}
uint64 getTime() { return time(NULL); }

void printFloat32(float32 data) {
    AlignRsp
    printf("%.10f", data); 
}

