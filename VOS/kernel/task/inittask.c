#include "desc.h"
#include "syscall.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();



u64 init(u64 arg) {
    printk(RED, BLACK, "init is running, arg = %#018lx\n", arg);
    Page *usrStkPage = Buddy_alloc(3, Page_Flag_Active);
    Task_switchToUsr(Task_initUsrLevel, (u64)DMAS_phys2Virt(usrStkPage->phyAddr), 114514);
    return 1;
}

u64 Task_doExit(u64 arg) {
    printk(RED, BLACK, "Task_doExit is running, arg = %#018lx\n", arg);
    while (1);
}

void Init_task() {
}