#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"

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

    loadTR(10);
    setTSS64(
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    Init_systemVector();
    Init_memManage();

    Buddy_debugLog(11);
    Page *page = Buddy_alloc(3, Page_Flag_Kernel | Page_Flag_Active);
    Page *page2 = Buddy_alloc(4, Page_Flag_Kernel | Page_Flag_Active);
    Buddy_free(page);
    Buddy_free(page2);
    Buddy_debugLog(11);

    // Page *page = BsMemManage_alloc(64, Page_Flag_Kernel | Page_Flag_Active);
    // for (int i = 0; i < 64; i++) {
    //     printk(WHITE, BLACK, "page[%d]: phyAddr = %#018lx, attr = %#018lx\n", i, page[i].phyAddr, page[i].attr);
    // }
    while (1) ;
}