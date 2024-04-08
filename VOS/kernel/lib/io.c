#include "io.h"

u8 IO_in8(u16 port) {
    u8 ret = 0;
    __asm__ __volatile__ (
        "inb %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

u32 IO_in32(u16 port) {
    u32 ret = 0;
    __asm__ __volatile__ (
        "inl %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}