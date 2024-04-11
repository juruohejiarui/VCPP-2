#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"

void startKernel() {

    position.XResolution = bootParamInfo->graphicsInfo.HorizontalResolution;
	position.YResolution = bootParamInfo->graphicsInfo.VerticalResolution;
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

    u64 *bs = (u64 *)(0xffff800040100000);
    for (int i = 0; i < 1000; i++) printk(WHITE, BLACK, "%#018lx\n", *(bs + i));
    while (1) ;
}