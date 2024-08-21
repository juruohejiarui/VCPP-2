#include "includes/lib.h"
#include "includes/log.h"
#include "includes/hardware.h"
#include "includes/interrupt.h"
#include "includes/memory.h"
#include "includes/task.h"
#include "includes/hardware.h"
#include "includes/smp.h"
#include "includes/simd.h"


u8 Init_stack[32768] __attribute__((__section__ (".data.Init_stack") )) = { 0 };

void startKernel() {
    position.XResolution = HW_UEFI_bootParamInfo->graphicsInfo.HorizontalResolution & 0xffff;
	position.YResolution = HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution & 0xffff;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;
 
    Log_init();

    printk(WHITE, BLACK, "FrameBufferBase: %#018lx, FrameBufferSize: %#018lx, HorizontalResolution: %#08lx, VerticalResolution: %#08x, PixelsPerScanLine: %#08x\n",
        HW_UEFI_bootParamInfo->graphicsInfo.FrameBufferBase, 		HW_UEFI_bootParamInfo->graphicsInfo.FrameBufferSize,
        HW_UEFI_bootParamInfo->graphicsInfo.HorizontalResolution, 	HW_UEFI_bootParamInfo->graphicsInfo.VerticalResolution,
        HW_UEFI_bootParamInfo->graphicsInfo.PixelsPerScanLine);
	printk(WHITE, BLACK, "Init_stack: %#018lx\n", Init_stack);
    Intr_Gate_loadTR(10);
    Intr_Gate_setTSS(
            tss64Table,
            (u64)(Init_stack + 32768), (u64)(Init_stack + 32768), (u64)(Init_stack + 32768), 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    Intr_Trap_setSysVec();
    MM_init();
    Log_enableBuf();

    Intr_init();

    HW_init();

	SIMD_init();
    SMP_init();
    
    Task_Syscall_init();
    Task_init();
    
    while (1) ;
}