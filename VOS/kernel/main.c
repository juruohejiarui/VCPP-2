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

    Gate_loadTR(10);
    Gate_setTSS(
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    Init_systemVector();
    Init_memManage();

    Init_interrupt();

    Init_CPU();

    Init_syscall();
    Init_task();
    
    while (1) ;
}