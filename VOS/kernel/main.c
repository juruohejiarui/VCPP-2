#include "includes/printk.h"
#include "includes/trap.h"
#include "includes/gate.h"
#include "includes/UEFI.h"
#include "includes/memory.h"
#include "includes/interrupt.h"
#include "includes/task.h"
#include "includes/cpu.h"

#include "memory/memoryinner.h"

struct KernelBootParameterInfo *bootParamInfo = (struct KernelBootParameterInfo *)0xffff800000060000;

void Start_Kernel(void) {
    memset((void *)&_bss, 0, (u64)&_ebss - (u64)&_bss);
    char ch = '\0', i;
    unsigned int fcol = 0x00ffffff, bcol = 0x000000;
    // position.XResolution = 3840;
    // position.YResolution = 2560;
    position.XResolution = bootParamInfo->graphicsInfo.HorizontalResolution;
	position.YResolution = bootParamInfo->graphicsInfo.VerticalResolution;
    position.XCharSize = 8;
    position.YCharSize = 16;

    position.XPosition = position.YPosition = 0;
    position.FBAddr = (unsigned int *)0xffff800003000000;
    
    loadTR(10);
    setTSS64(_stack_start, _stack_start, _stack_start, 
        0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    systemVectorInit();

    initCPU();

    initMemory();

    Page *page = Buddy_alloc(0, 0);
    Buddy_debug();
    PageTable_initPageTable(page->phyAddr, (u64 *)0x0000000000000000);
    setCR3(page->phyAddr + sizeof(u64));

    initInterrupt();

    Task_init();

    // Page *pages = allocPages(ZONE_NORMAL, 32, PAGE_Kernel | PAGE_PTable_Maped | PAGE_Active);
    // for (int i = 0; i < 64; i++)
    //     printk(WHITE, BLACK, "Page %d: %p attribute: %lld, phyAddr: %#018lx\n", i, pages + i, (pages + i)->attribute, (pages + i)->phyAddr);

    // i = 1 / 0;
    // i = *(unsigned char *)(0xffff80000aa00000);
    while (1) ;
}