#include "desc.h"
#include "syscall.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

u64 init(u64 (*usrEntry)(u64), u64 arg) {
    if (Task_current->pid == 0) {
        List_del(&Init_taskStruct.listEle);
        APIC_enableIntr(0x12);
        APIC_enableIntr(0x14);
        IO_sti();
    }
	Task_current->state = Task_State_Running;
    printk(RED, BLACK, "init is running, arg = %#018lx\n", arg);
    Task_switchToUsr(usrEntry, arg);
    return 1;
}

u64 usrInit(u64 arg) {
	printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    u64 res = Syscall_usrAPI(arg, BLACK, WHITE, (u64)"Up Down Up Down baba", 20, 5);
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    int t = 10000000 * (arg + 3), initCounter = 100000000;
    int tmp = arg;
    while (tmp > 0) initCounter <<= 1, tmp--;
    if (arg == 1) initCounter = t = 1;
    while (1) {
        if (arg == 1) 
			Syscall_usrAPI(0, 0x14, 2, 3, 4, 5);
        if (--t == 0) {
			printk(WHITE, BLACK, "%d ", Task_current->pid);
			arg == 1 ? Syscall_usrAPI(3, 0, 0, 0, 0, 0) : 0;
			t = initCounter;
		}
    }
    while (1) ;
}

u64 Task_doExit(u64 arg) {
    printk(RED, BLACK, "Task_doExit is running, arg = %#018lx\n", arg);
    while (1);
}

void Init_task() {
    Task_pidCounter = 0;
	// fake the task struction of the current task
    Init_taskStruct.thread->rsp0 = Init_taskStruct.thread->rsp = 0xffff800000007E00;
    Init_taskStruct.thread->fs = Init_taskStruct.thread->gs = Segment_kernelData;
    List_init(&Init_taskStruct.listEle);
    
    TaskStruct *initTask[3] = { NULL };
    for (int i = 0; i < 3; i++)
        initTask[i] = Task_createTask(init, usrInit, i, Task_Flag_Kernel);
    List_del(&Init_taskStruct.listEle);
    Task_switchTo(&Init_taskStruct, initTask[0]);
}