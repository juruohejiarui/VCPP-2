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

void IO_writeMSR(u64 msrAddr, u64 data) { 
    __asm__ __volatile__ (
        "wrmsr \n\t"
        :
        : "c"(msrAddr), "A"(data & 0xFFFFFFFF), "d"((data >> 32) & 0xFFFFFFFF)
        : "memory"
    );
}

u64 IO_readMSR(u64 msrAddr) {
    u32 data1, data2;
    __asm__ __volatile__ (
        "rdmsr \n\t"
        : "=d"(data1), "=a"(data2)
        : "c"(msrAddr)
        : "memory"
    );
    return (((u64) data1) << 32) | data2;
}