#include "APIC.h"
#include "cpu.h"
#include "../includes/log.h"
#include "../includes/memory.h"

Page *apicRegPage = NULL;

struct APIC_IOAddrMap {
    u32 phyAddr;
    u8 *virtIndexAddr;
    u32 *virtDataAddr;
    u32 *virtEOIAddr;
} APIC_ioMap;

u64 Hardware_APIC_getReg_IA32_APIC_BASE() { return IO_readMSR(0x1b); }

void HW_APIC_setReg_IA32_APIC_BASE(u64 value) { IO_writeMSR(0x1b, value); }
void APIC_setReg_IA32_APIC_BASE_address(u64 phyAddr) { 
    HW_APIC_setReg_IA32_APIC_BASE(phyAddr | Hardware_APIC_getReg_IA32_APIC_BASE() & ((1ul << 12) - 1)); 
}
void APIC_initLocal() {
    printk(RED, BLACK, "APIC_initLocal()\n");
    apicRegPage = (Page *)MM_Buddy_alloc(0, Page_Flag_Kernel | Page_Flag_KernelShare);
    memset(DMAS_phys2Virt(apicRegPage->phyAddr), 0, Page_4KSize);
    APIC_setReg_IA32_APIC_BASE_address(apicRegPage->phyAddr);
    u32 eax, ebx, ecx, edx;
    HW_CPU_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    printk(WHITE, BLACK, "CPUID\t01, eax: %#010x, ebx: %#010x, ecx: %#010x, edx: %#010x\n", eax, ebx, ecx, edx);

    // check the support of APIC & xAPIC
    if ((1 << 9) & edx) printk(WHITE, BLACK, "HW support for APIC & xAPIC\n");
    else printk(WHITE, BLACK, "No HW support for APIC & xAPIC\n");
    // check the support of x2APIC
    if ((1 << 21) & ecx) printk(WHITE, BLACK, "HW support for x2APIC\n");
    else printk(WHITE, BLACK, "No HW support for x2APIC\n");

    // enable xAPIC & x2APIC
    u32 x, y;
    __asm__ volatile (
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
    if (0xC00 & x) printk(WHITE, BLACK, "xAPIC & x2APIC enabled\n");

    // enable SVR[8] & SVR[12]
    __asm__ volatile (
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
    if (0x100 & x) printk(WHITE, BLACK, "SVR[8] enabled\n");
    if (0x1000 &x) printk(WHITE, BLACK, "SVR[12] enabled\n");

    // get local APIC ID
    __asm__ volatile (
        "movl $0x802, %%ecx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "Local APIC ID: %d\n", x);

    // get local APIC version
    __asm__ volatile (
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

    *(u64 *)DMAS_phys2Virt(0xfee002f0) = 0x100000;
    for (u64 i = 0x320; i <= 0x370; i += 0x10) *(u64 *)DMAS_phys2Virt(0xfee00000 + i) = 0x100000;
    
    // get TPR
    __asm__ volatile (
        "movq $0x808, %%rcx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "TPR: %#010x\t", x & 0xff);

    // get PPR
    __asm__ volatile (
        "movq $0x80a, %%rcx           \n\t"
        "rdmsr                        \n\t"
        : "=a"(x), "=d"(y)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "PPR: %#010x\n", x & 0xff);
}

void APIC_mapIOAddr() {
    APIC_ioMap.phyAddr = 0xfec00000;
    
    printk(WHITE, BLACK, "APIC IO Address: %#08x\n", APIC_ioMap.phyAddr);
    APIC_ioMap.virtIndexAddr = (u8 *)DMAS_phys2Virt(APIC_ioMap.phyAddr);
    APIC_ioMap.virtDataAddr = (u32 *)(APIC_ioMap.virtIndexAddr + 0x10);
    APIC_ioMap.virtEOIAddr = (u32 *)(APIC_ioMap.virtIndexAddr + 0x40);
}

u64 APIC_readRTE(u8 index) {
    u64 ret;
    *APIC_ioMap.virtIndexAddr = index + 1;
    IO_mfence();
    ret = *APIC_ioMap.virtDataAddr;
    ret <<= 32;
    IO_mfence();

    *APIC_ioMap.virtIndexAddr = index;
    IO_mfence();
    ret |= *APIC_ioMap.virtDataAddr;
    IO_mfence();

    return ret;
}

void HW_APIC_writeRTE(u8 index, u64 val) {
    *APIC_ioMap.virtIndexAddr = index;
    IO_mfence();
    *APIC_ioMap.virtDataAddr = val & 0xffffffff;
    IO_mfence();

    *APIC_ioMap.virtIndexAddr = index + 1;
    IO_mfence();
    *APIC_ioMap.virtDataAddr = val >> 32;
    IO_mfence();
}



int enable[0x40] = {0};

#define handlerId(intrId) (((intrId - 0x20) << 1) + 0x10)

void HW_APIC_suspend() {
    for (int i = 0x10; i < 0x40; i += 2) if (enable[i]) HW_APIC_disableIntr(i);
}
void HW_APIC_resume() {
    for (int i = 0x10; i < 0x40; i += 2) if (enable[i]) HW_APIC_enableIntr(i);
}

void HW_APIC_disableIntr(u8 intrId) {
    u64 val = APIC_readRTE(handlerId(intrId));
    val |= 0x10000ul;
    HW_APIC_writeRTE(handlerId(intrId), val);
    enable[handlerId(intrId)] = 0;
}

void HW_APIC_enableIntr(u8 intrId) {
    u64 val = APIC_readRTE(handlerId(intrId));
    val &= ~0x10000ul;
    HW_APIC_writeRTE(handlerId(intrId), val);
    enable[handlerId(intrId)] = 1;
}

void HW_APIC_install(u8 intrId, void *arg) {
    APICRteDescriptor *desc = (APICRteDescriptor *)arg;
    HW_APIC_writeRTE(handlerId(intrId), *(u64 *)desc);
    enable[handlerId(intrId)] = 1;
}

void HW_APIC_uninstall(u8 intrId) {
    HW_APIC_writeRTE(handlerId(intrId), 0x10000ul);
    enable[handlerId(intrId)] = 0;
}

void HW_APIC_edgeAck(u8 irqId) {
	// write the EOI register to annouce that the interrupt has been handled
    __asm__ volatile (
        "movq $0x00, %%rdx  	\n\t" \
        "movq $0x00, %%rax  	\n\t" \
        "movq $0x80b, %%rcx 	\n\t" \
        "wrmsr              \n\t" \
        :
        :
        : "memory"
    );
}
void APIC_initIO() {
    *APIC_ioMap.virtIndexAddr = 0x00;
    IO_mfence();
    *APIC_ioMap.virtDataAddr = 0x0f000000;
    IO_mfence();
    printk(WHITE, BLACK, "IOAPIC ID REG: %#010x ID: %#010x\n", *APIC_ioMap.virtDataAddr, *APIC_ioMap.virtDataAddr >> 24 & 0xf);
    IO_mfence();

    *APIC_ioMap.virtIndexAddr = 0x01;
    IO_mfence();
    printk(WHITE, BLACK, "IOAPIC VER REG: %#010x VER: %#010x\n", *APIC_ioMap.virtDataAddr, ((*APIC_ioMap.virtDataAddr >> 16) & 0xff) + 1);
    printk(GREEN, BLACK, "IOAPIC Redirection Table Entries initialized\n");
}

int APIC_flag = 0;

void HW_APIC_init() {
    printk(RED, BLACK, "HW_APIC_init()\n");
    APIC_mapIOAddr();
    
    IO_out8(0x21, 0xff);
    IO_out8(0xa1, 0xff);


    IO_out8(0x22, 0x70);
    IO_out8(0x23, 0x01);

    APIC_initLocal();
    APIC_initIO();

    IO_out32(0xcf8, 0x8000f8f0);
    IO_mfence();
    APIC_flag = 1;

    IO_sti();
}

int HW_APIC_finishedInit() { return APIC_flag; }
