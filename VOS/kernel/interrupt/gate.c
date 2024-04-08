#include "gate.h"

#define setGate(idtAddr, attr, istIndex, codeAddr) \
    do { \
        u64 d0, d1; \
        __asm__ __volatile__ ( \
            /* set the 0...15-th bits of code addr */ \
            "movw %%dx, %%ax        \n\t" \
            /* set the ist index and attr */ \
            "andq $0x7, %%rcx       \n\t" \
            "addq %4, %%rcx         \n\t" \
            "shlq $32, %%rcx         \n\t" \
            "orq %%rcx, %%rax       \n\t" \
            /* set the 15...31-th bits of code addr to 48...63-th bits */ \
            "xorq %%rcx, %%rcx      \n\t" \
            "movl %%edx, %%ecx      \n\t" \
            "shrq $16, %%rcx        \n\t" \
            "shlq $48, %%rcx        \n\t" \
            "orq %%rcx, %%rax       \n\t" \
            "movq %%rax, %0         \n\t" \
            /* set the 32...63-th bits of code addr to 64...95-th bits */ \
            "shrq $32, %%rdx        \n\t" \
            "movq %%rdx, %1         \n\t" \
            :   "=m"(*(u64 *)(idtAddr)), "=m"(*(1 + (u64 *)(idtAddr))), \
                "=&a"(d0), "=&d"(d1) \
            :   "i"(attr << 8), \
                "3"((u64 *)(codeAddr)),  /*set rdx to be the code addr*/ \
                "2"(0x8 << 16), /*set the segment selector to be 0x8 (kernel code 64-bit)*/ \
                "c"(istIndex) \
            : "memory" \
        ); \
    } while(0)

void setIntrGate(u64 idtIndex, u8 istIndex, void *codeAddr) {
    setGate(idtTable + idtIndex, 0x8E, istIndex, codeAddr);
}

void setTrapGate(u64 idtIndex, u8 istIndex, void *codeAddr) {
    setGate(idtTable + idtIndex, 0x8F, istIndex, codeAddr);
}

void setSystemGate(u64 idtIndex, u8 istIndex, void *codeAddr) {
    setGate(idtTable + idtIndex, 0xEF, istIndex, codeAddr);
}

void setSystemIntrGate(u64 idtIndex, u8 istIndex, void *codeAddr) {
    setGate(idtTable + idtIndex, 0xEE, istIndex, codeAddr);
}

void setTSS64(
        u64 rsp0, u64 rsp1, u64 rsp2, u64 ist1, u64 ist2, 
        u64 ist3, u64 ist4, u64 ist5, u64 ist6, u64 ist7) {
    *(u64 *)(tss64Table + 1) = rsp0;
    *(u64 *)(tss64Table + 3) = rsp1;
    *(u64 *)(tss64Table + 5) = rsp2;
    *(u64 *)(tss64Table + 9) = ist1;
    *(u64 *)(tss64Table + 11) = ist2;
    *(u64 *)(tss64Table + 13) = ist3;
    *(u64 *)(tss64Table + 15) = ist4;
    *(u64 *)(tss64Table + 17) = ist5;
    *(u64 *)(tss64Table + 19) = ist6;
    *(u64 *)(tss64Table + 21) = ist7;
}