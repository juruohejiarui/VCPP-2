#include "cpu.h"
#include "../includes/log.h"

void HW_CPU_cpuid(u32 mop, u32 sop, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx) {
    __asm__ volatile (
        "cpuid \n\t"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "0"(mop), "2"(sop)
    );
}

void HW_CPU_init() {
    printk(RED, BLACK, "HW_CPU_init()\n");
    int i, j;
    u32 cpuFacName[4] = {0, 0, 0, 0};
    char facName[17] = {0};

    HW_CPU_cpuid(0, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
    *(u32 *)&facName[0] = cpuFacName[1];
    *(u32 *)&facName[4] = cpuFacName[3];
    *(u32 *)&facName[8] = cpuFacName[2];
    facName[12] = '\0';
    printk(YELLOW, BLACK, "CPU: %s\t%#010x\t%#010x\t%#010x\n", facName, cpuFacName[1], cpuFacName[3], cpuFacName[2]);

    for (i = 0x80000002; i <= 0x80000004; i++) {
        HW_CPU_cpuid(i, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
        for (j = 0; j < 4; j++)
            *(u32 *)&facName[j * 4] = cpuFacName[j];
        facName[16] = '\0';
        printk(WHITE, BLACK, "%s", facName);
    }
    putchar(WHITE, BLACK, '\n');
    // version information type, family, model, and stepping id
    HW_CPU_cpuid(1, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
    printk(WHITE, BLACK, 
        "Family Code: %#010x, Extended Family Code: %#010x, Model Number: %#010x, Extended Model Number: %#010x Processor Type: %#010x Stepping ID: %#010x\n",
        (cpuFacName[0] >> 8) & 0xf, (cpuFacName[0] >> 20) & 0xff, (cpuFacName[0] >> 4) & 0xf, (cpuFacName[0] >> 16) & 0xf, (cpuFacName[0] >> 12) & 0x3, cpuFacName[0] & 0xf);
    // linear/physical address size
    HW_CPU_cpuid(0x80000008, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
    printk(WHITE, BLACK, "Physical Address Size: %d, Linear Address Size: %d\n", cpuFacName[0] & 0xff, (cpuFacName[0] >> 8) & 0xff);

    // max cpuid operation code
    HW_CPU_cpuid(0, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
    printk(WHITE, BLACK, "Max Cpuid Operation Code: %#010x\n", cpuFacName[0]);

    HW_CPU_cpuid(0x80000000, 0, &cpuFacName[0], &cpuFacName[1], &cpuFacName[2], &cpuFacName[3]);
    printk(WHITE, BLACK, "Max Extended Cpuid Operation Code: %#010x\n", cpuFacName[0]);
}