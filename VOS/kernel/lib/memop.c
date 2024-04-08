#include "memop.h"

void *memset(void *addr, u8 dt, u64 size) {
    int d0, d1;
    u64 tmp = dt * 0x0101010101010101UL;
    __asm__ __volatile__ (
        "cld                    \n\t"
        "rep                    \n\t"
        "stosq                  \n\t"
        "testb $4, %b3          \n\t"
        "je 1f                  \n\t"
        "stosl                  \n\t"
        "1:\ttestb $2, %b3      \n\t"
        "je 2f                  \n\t"
        "stosw                  \n\t"
        "2:\ttestb $1, %b3      \n\t"
        "je 3f                  \n\t"
        "stosb                  \n\t"
        "3:                     \n\t"
        : "=&c"(d0), "=&D"(d1)
        : "a"(tmp), "q"(size), "0"(size >> 3), "1"(addr)
        : "memory"
    );
    return addr;
}

void *memcpy(void *src, void *dst, u64 size) {
    int d0, d1, d2;
    __asm__ __volatile__ (
        "cld                    \n\t"
        "movsq                  \n\t"
        "testb $4, %b4          \n\t"
        "je 1f                  \n\t"
        "movsl                  \n\t"
        "1:\ttestb $2, %b4      \n\t"
        "je 2f                  \n\t"
        "movsw                  \n\t"
        "2:\ttestb $1, %b4      \n\t"
        "je 3f                  \n\t"
        "3:                     \n\t"
        : "=&c"(d0), "=&D"(d1), "=&S"(d2)
        : "0"(size / 8), "q"(size), "1"(dst), "2"(src)
        : "memory"
    );
    return dst;
}

int memcmp(void *fir, void *sec, u64 size) {
    register int res;
    __asm__ __volatile__ (
        "cld            \n\t"
        "repe           \n\t"
        "cmpsb          \n\t"
        "je 1f          \n\t"
        "movl $1, %%eax \n\t"
        "jl 1f          \n\t"
        "negl %%eax     \n\t"
        "1:             \n\t"
        : "=a"(res)
        : "0"(0), "D"(fir), "S"(sec), "c"(size)
        :
    );
    return res;
}

i64 strlen(u8 *str) {
    register i64 res;
    __asm__ __volatile__ (
        "cld        \n\t"
        "repne      \n\t"
        "scasb      \n\t"
        "notl %0    \n\t"
        "decl %0    \n\t"
        : "=c"(res)
        : "D"(str), "a"(0), "0"(0xffffffffffffffff)
    );
    return res;
}