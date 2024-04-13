#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"

void drawPoint(int x, int y, unsigned int color) {
    position.FBAddr[x + y * position.XResolution] = color;
}

void startKernel() {
    position.XResolution = bootParamInfo->graphicsInfo.HorizontalResolution;
	position.YResolution = bootParamInfo->graphicsInfo.VerticalResolution;
    // position.XResolution = 3840;
	// position.YResolution = 2560;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;
    printk(RED, BLACK, "hello world\n");

    loadTR(10);
    setTSS64(
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    Init_systemVector();
    Init_memManage();

    // Page *page = BsMemManage_alloc(64, Page_Flag_Kernel | Page_Flag_Active);
    // for (int i = 0; i < 64; i++) {
    //     printk(WHITE, BLACK, "page[%d]: phyAddr = %#018lx, attr = %#018lx\n", i, page[i].phyAddr, page[i].attr);
    // }
    while (1) ;
}