#include "io.h"

void IO_out8(u16 port, u8 data) {
    __asm__ volatile (
        "outb %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

void IO_out16(u16 port, u16 data) {
    __asm__ volatile (
        "outw %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

void IO_out32(u16 port, u32 data) {
    __asm__ volatile (
        "outl %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

u8 IO_in8(u16 port) {
    u8 ret = 0;
    __asm__ volatile (
        "inb %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

u16 IO_in16(u16 port) {
    u16 ret = 0;
    __asm__ volatile (
        "inw %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

u32 IO_in32(u16 port) {
    u32 ret = 0;
    __asm__ volatile (
        "inl %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

void IO_writeMSR(u64 msrAddr, u64 data) { 
    __asm__ volatile (
        "wrmsr \n\t"
        :
        : "c"(msrAddr), "A"(data & 0xFFFFFFFF), "d"((data >> 32) & 0xFFFFFFFF)
        : "memory"
    );
}

u64 IO_readMSR(u64 msrAddr) {
    u32 data1, data2;
    __asm__ volatile (
        "rdmsr \n\t"
        : "=d"(data1), "=a"(data2)
        : "c"(msrAddr)
        : "memory"
    );
    return (((u64) data1) << 32) | data2;
}

u64 IO_getRIP() {
    u64 ret = 0;
    __asm__ volatile (
        "lea (%%rip), %0 \n\t"
        : "=r"(ret)
    );
    return ret;
}