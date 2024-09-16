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
    Intr_Gate_setTSS(
            tss64Table,
            (u64)(Init_stack + 32768), (u64)(Init_stack + 32768), (u64)(Init_stack + 32768), 0xffff800000007c00, 0xffff800000007c00,
            0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);
	Intr_Gate_loadTR(10);
    Intr_Trap_setSysVec();

    SMP_cpuNum = 0;
	memset(SMP_cpuInfo, 0, sizeof(SMP_cpuInfo));
    Task_cfsStruct.flags = 0;
 
    Log_init();

    MM_init();

    Log_enableBuf();

    Intr_init();

    HW_init();

    SMP_init();
	SIMD_init();
    
    Task_Syscall_init();
    Task_init();
    
    while (1) ;
}