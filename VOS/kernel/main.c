#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"
#include "includes/task.h"
#include "includes/hardware.h"

void drawPoint(int x, int y, unsigned int color) {
    position.FBAddr[x + y * position.XResolution] = color;
}

void startKernel() {
    position.XResolution = HW_UEFI_bootParamInfo->graphicsInfo.HorizontalResolution & 0xffff;
	position.YResolution = HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution & 0xffff;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;

    printk(RED, BLACK, "hello world\n");

    printk(RED, BLACK, "FrameBufferBase: %#018lx, FrameBufferSize: %#018lx, HorizontalResolution: %#08lx, VerticalResolution: %#08x, PixelsPerScanLine: %#08x\n",
        HW_UEFI_bootParamInfo->graphicsInfo.FrameBufferBase, 		HW_UEFI_bootParamInfo->graphicsInfo.FrameBufferSize,
        HW_UEFI_bootParamInfo->graphicsInfo.HorizontalResolution, 	HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution,
        HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine);

    printk(RED, BLACK, "availAddrSt = %#018lx\n", availVirtAddrSt);

    Intr_Gate_loadTR(10);
    Intr_Gate_setTSS(
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	printk(BLACK, WHITE, "There are %d entries in configuration table\n", HW_UEFI_bootParamInfo->ConfigurationTableCount);
    Intr_Trap_setSysVec();
    MM_init();

    Intr_init();
    HW_init();

    CMOSDateTime dateTime;
    HW_Timer_CMOS_getDateTime(&dateTime);
    printk(RED, BLACK, "Current time: %x-%x-%x %x:%x:%x\n", dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second);

    printk(WHITE, BLACK, "%#018lx, %#018lx\n", memberOffset(TaskStruct, state), 0);
    
    Task_Syscall_init();
    Task_init();
    
    while (1) ;
}