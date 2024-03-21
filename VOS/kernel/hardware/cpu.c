#include "../includes/cpu.h"
#include "../includes/printk.h"

void getCPUID(u32 mop, u32 sop, u32 *a, u32 *b, u32 *c, u32 *d) {
    __asm__ __volatile__ (
        "cpuid \n\t"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "0"(mop), "2"(sop)
    );
}

void initCPU() {
    u32 cpuFacName[4] = {0, 0, 0, 0};
    u8 factoryName[17] = {0};

    getCPUID(0, 0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);
    *(u32 *)(factoryName + 0) = cpuFacName[1];
    *(u32 *)(factoryName + 4) = cpuFacName[3];
    *(u32 *)(factoryName + 8) = cpuFacName[2];
    *(u32 *)(factoryName + 12) = '\0';
    printk(WHITE, BLACK, "%s\t%#010lx\t%#010lx\t%#010lx\n", factoryName, cpuFacName[1], cpuFacName[3], cpuFacName[2]);

    for (int i = 0x80000002; i <= 0x80000005; i++) {
        getCPUID(i, 0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);

        for (int j = 0; j < 4; j++) 
            *((u32 *)factoryName + j) = cpuFacName[j];
        factoryName[16] = '\0';
        printk(YELLOW, BLACK, "%s", factoryName);
    }
    putchar(WHITE, BLACK, '\n');
    // version information type, family, model, and stepping id
    getCPUID(1, 0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);
    printk(ORANGE, BLACK, "Family Code: %#010lx, Extended Family: %#010lx, Model Number: %#010lx, Processor Type: %#010lx, Stepping ID: %#010lx\n",
        (cpuFacName[0] >> 8 & 0xf), (cpuFacName[0] >> 20 & 0xf), (cpuFacName[0] >> 4 & 0xf), 
        (cpuFacName[0] >> 16 & 0xf), (cpuFacName[0] >> 12 & 0xf), (cpuFacName[0] & 0xf));

    // get linear/physical address size
    getCPUID(0x80000008,0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);
    printk(WHITE, BLACK, "Physical address size: %08d, Linear Address size: %08d\n",
        cpuFacName[0] & 0xff, cpuFacName[0] >> 8 & 0xff);
    
    // max cpuid operation code
    getCPUID(0, 0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);
    printk(WHITE, BLACK, "MAX Basic Operation Code: %#010x\t", cpuFacName[0]);
    getCPUID(0x80000000, 0, cpuFacName + 0, cpuFacName + 1, cpuFacName + 2, cpuFacName + 3);
    printk(WHITE, BLACK, "MAX Extended Operation Code: %#010d\t", cpuFacName[0]);
}
