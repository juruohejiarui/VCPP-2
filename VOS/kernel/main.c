#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"
#include "includes/task.h"

void drawPoint(int x, int y, unsigned int color) {
    position.FBAddr[x + y * position.XResolution] = color;
}

void startKernel() {
    position.XResolution = bootParamInfo->graphicsInfo.HorizontalResolution & 0xffff;
	position.YResolution = bootParamInfo->graphicsInfo.VerticalResolution & 0xffff;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;

    printk(RED, BLACK, "hello world\n");

    printk(RED, BLACK, "FrameBufferBase: %#018lx, FrameBufferSize: %#018lx, HorizontalResolution: %#08lx, VerticalResolution: %#08x, PixelsPerScanLine: %#08x\n",
        bootParamInfo->graphicsInfo.FrameBufferBase, bootParamInfo->graphicsInfo.FrameBufferSize,
        bootParamInfo->graphicsInfo.HorizontalResolution, bootParamInfo->graphicsInfo.VerticalResolution,
        bootParamInfo->graphicsInfo.PixelsPerScanLine);

    printk(RED, BLACK, "availAddrSt = %#018lx\n", availVirtAddrSt);

    Intr_Gate_loadTR(10);
    Intr_Gate_setTSS(
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	printk(BLACK, WHITE, "There are %d entries in configuration table\n", bootParamInfo->ConfigurationTableCount);
    Intr_Trap_setSysVec();
    MM_init();

    Intr_init();

    HW_CPU_init();

    Task_Syscall_init();
    Init_task();
    
    while (1) ;
}