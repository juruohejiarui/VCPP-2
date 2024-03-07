#include "includes/printk.h"
#include "includes/trap.h"
#include "includes/gate.h"
#include "includes/UEFI.h"
#include "includes/memory.h"
#include "includes/interrupt.h"

struct KernelBootParameterInfo *bootParamInfo = (struct KernelBootParameterInfo *)0xffff800000060000;

void Start_Kernel(void) {
    char ch = '\0', i;
    unsigned int fcol = 0x00ffffff, bcol = 0x000000;
    position.XResolution = bootParamInfo->graphicsInfo.HorizontalResolution;
	position.YResolution = bootParamInfo->graphicsInfo.VerticalResolution;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;

    
    loadTR(10);
    setTSS64(   0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
                0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
    systemVectorInit();

    initMemory();

    initInterrupt();

    // i = 1 / 0;
    // i = *(unsigned char *)(0xffff80000aa00000);
    while (1) ;
}