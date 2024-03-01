#include "syscall.h"
#include "vgvm.h"

uint64 syscallArgCnt[] = {1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1 };
void *syscall[] = {VMputchar, VMgetchar, printInt, inputInt, printInt64, inputInt64, printUInt64, inputUInt64, getTime, NULL, printFloat32 };

void VMputchar(char ch) { putchar(ch); }
char VMgetchar() { return getchar(); }
void printInt(int32 data) {
    AlignRsp
    printf("%d", data); }
int32 inputInt() { 
    AlignRsp
    int32 data; scanf("%d", &data); return data; 
}
void printInt64(int64 data) {
    AlignRsp
    printf("%lld", data);
}
int64 inputInt64() { 
    AlignRsp
    int64 data; scanf("%lld", &data); return data; 
}
void printUInt64(uint64 data) {
    AlignRsp
    printf("%llu", data); 
}
uint64 inputUInt64() { uint64 data;
    AlignRsp
    scanf("%llu", &data); return data; 
}
uint64 getTime() { return time(NULL); }

void printFloat32(uint64 data) {
    AlignRsp
    printf("%.10f", *(float32 *)&data); 
}