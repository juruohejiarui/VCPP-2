#include "APIC.h"
#include "cpu.h"
#include "../includes/log.h"
#include "../includes/memory.h"

Page *apicRegPage = NULL;

u64 APIC_getReg_IA32_APIC_BASE() { return IO_readMSR(0x1b); }

void APIC_setReg_IA32_APIC_BASE(u64 value) { IO_writeMSR(0x1b, value); }
void APIC_setReg_IA32_APIC_BASE_address(u64 phyAddr) { 
    APIC_setReg_IA32_APIC_BASE(phyAddr | APIC_getReg_IA32_APIC_BASE() & ((1ul << 12) - 1)); 
}
void APIC_initLocal() {
    apicRegPage = (Page *)Buddy_alloc(0, Page_Flag_Kernel);
    memset(DMAS_phys2Virt(apicRegPage->phyAddr), 0, Page_4KSize);
    APIC_setReg_IA32_APIC_BASE_address(apicRegPage->phyAddr);
    u32 eax, ebx, ecx, edx;
    CPU_getID(1, 0, &eax, &ebx, &ecx, &edx);
    printk(WHITE, BLACK, "CPUID\t01, eax: %#010x, ebx: %#010x, ecx: %#010x, edx: %#010x\n", eax, ebx, ecx, edx);

    // check the support of APIC & xAPIC
    if ((1 << 9) & edx) printk(WHITE, BLACK, "HW support for APIC & xAPIC\n");
    else printk(WHITE, BLACK, "No HW support for APIC & xAPIC\n");
    // check the support of x2APIC
    if ((1 << 21) & ecx) printk(WHITE, BLACK, "HW support for x2APIC\n");
    else printk(WHITE, BLACK, "No HW support for x2APIC\n");

    // enable xAPIC & x2APIC
    u32 x, y;
    __asm__ __volatile__ (
        "movq $0x1b, %%rcx          \n\t"
        "rdmsr                      \n\t"
        "btsq $10, %%rax            \n\t" // set the bits of xAPIC & x2APIC
        "btsq $11, %%rax            \n\t"
        "wrmsr                      \n\t"
        "movq $0x1b, %%rcx          \n\t"
        "rdmsr                      \n\t"
        : "=a"(x), "=d"(y)
        : 
        : "memory"
    );
    printk(WHITE, BLACK, "eax: %#010x, edx: %#010x\n", x, y);
    if (0xC00 & x) printk(WHITE, BLACK, "xAPIC & x2APIC enabled\n");

    // enable SVR[8] & SVR[12]
    __asm__ __volatile__ (
        "movq $0x80f, %%rcx         \n\t"
        "rdmsr                      \n\t"
        "btsq $8, %%rax             \n\t"
        "btsq $12, %%rax            \n\t"
        "wrmsr                      \n\t"
        "movq $0x80f, %%rcx         \n\t"
        "rdmsr                      \n\t"
        : "=a"(x), "=d"(y)
        : 
        : "memory"
    );
    printk(WHITE, BLACK, "eax: %#010x, edx: %#010x\n", x, y);
    if (0x100 & x) printk(WHITE, BLACK, "SVR[8] enabled\n");
    if (0x1000 &x) printk(WHITE, BLACK, "SVR[12] enabled\n");

    // get local APIC ID
    __asm__ __volatile__ (
        "movl $0x802, %%ecx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "Local APIC ID: %d\n", x);

    // get local APIC version
    __asm__ __volatile__ (
        "movl $0x803, %%ecx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, 
            "Local APIC Version: %d, Max LVT Entry: %#010x, SVR(Suppress EOI Broadcast): %#04x\n", 
            x & 0xff, ((x >> 16) & 0xff) + 1, (x >> 24) & 0x1);
    if ((x % 0xff) < 0x10)
        printk(WHITE, BLACK, "82489DX discrete APIC\n");
    else if (((x & 0xff) >= 0x10) && ((x & 0xff) < 0x20))
        printk(WHITE, BLACK, "Integrated APIC\n");

    // mask all LVT entries
    __asm__ __volatile__ (
        "movq $0x82f, %%rcx           \n\t" // CMCI
        "wrmsr                        \n\t"
        "movq $0x832, %%rcx           \n\t" // Timer
        "wrmsr                        \n\t"
        "movq $0x833, %%rcx           \n\t" // Thermal Monitor
        "wrmsr                        \n\t"
        "movq $0x834, %%rcx           \n\t" // Performance Counter
        "wrmsr                        \n\t"
        "movq $0x835, %%rcx           \n\t" // LINT0
        "wrmsr                        \n\t"
        "movq $0x836, %%rcx           \n\t" // LINT1
        "wrmsr                        \n\t"
        "movq $0x837, %%rcx           \n\t" // LINT2
        "wrmsr                        \n\t"
        :
        : "a"(0x10000), "d"(0)
        : "memory"
    );
    
    // get TPR
    __asm__ __volatile__ (
        "movq $0x808, %%rcx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "TPR: %#010x\n", x & 0xff);

    // get PPR
    __asm__ __volatile__ (
        "movq $0x80a, %%rcx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "PPR: %#010x\n", x & 0xff);
}

void APIC_initIO() {
    IO_out8(0x21, 0xff);
    IO_out8(0xa1, 0xff);

    APIC_initLocal();
    sti();
}

void Init_APIC() {
    APIC_initIO();
}
