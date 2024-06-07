#include "desc.h"
#include "syscall.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

u64 init(u64 (*usrEntry)(u64), u64 arg) {
	u64 rsp = 0;
	__asm__ volatile ( "movq %%rsp, %0" : "=m"(rsp) : : "memory" );
    Intr_SoftIrq_Timer_initIrq(&Task_current->timerIrq, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->timerIrq);
	Task_current->state = Task_State_Running;
	printk(RED, BLACK, "init is running, arg = %#018lx, rsp = %#018lx\n", arg, rsp);
    if (Task_current->pid == 0) {
        printk(WHITE, BLACK, "task 0 is running...\n");
        IO_sti();
        for (KeyboardEvent *kpEvent; ; ) {
            kpEvent = HW_Keyboard_getEvent();
            if (kpEvent == NULL) continue;
            if (kpEvent->isCtrlKey) {
                if (!kpEvent->isKeyUp) printk(BLACK, YELLOW, "{%d}", kpEvent->keyCode);
                else printk(BLACK, RED, "{%d}", kpEvent->keyCode);
            } else {
                if (!kpEvent->isKeyUp) printk(BLACK, WHITE, "[%c]", kpEvent->keyCode);
            }
            kfree(kpEvent);
        }
    } else Task_switchToUsr(usrEntry, arg);
    return 1;
}

u64 usrInit(u64 arg) {
	printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    u64 res = Task_Syscall_usrAPI(arg, BLACK, WHITE, (u64)"Up Down Up Down baba", 20, 5);
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    int t = 10000000 * (arg + 3), initCounter = 100000000;
    int tmp = arg;
    while (tmp > 0) initCounter <<= 1, tmp--;
    if (arg == 1) initCounter = t = 1;
    while (1) {
        if (arg == 1) 
			Task_Syscall_usrAPI(0, 0x14, 2, 3, 4, 5);
        if (--t == 0) {
			printk(WHITE, BLACK, "%d ", arg);
			arg == 1 ? Task_Syscall_usrAPI(3, 0, 0, 0, 0, 0) : 0;
			t = initCounter;
		}
    }
    while (1) ;
}

u64 Task_doExit(u64 arg) {
    printk(RED, BLACK, "Task_doExit is running, arg = %#018lx\n", arg);
    while (1);
}

void Task_init() {
    printk(RED, BLACK, "Task_init()\n");
    Task_initMgr();
    Task_pidCounter = 0;
	// fake the task struction of the current task
    Init_taskStruct.thread->rsp0 = Init_taskStruct.thread->rsp = 0xffff800000007E00;
    Init_taskStruct.thread->fs = Init_taskStruct.thread->gs = Segment_kernelData;
    List_init(&Init_taskStruct.listEle);
    
    TaskStruct *initTask[3] = { NULL };
    for (int i = 0; i < 3; i++)
        initTask[i] = Task_createTask(init, usrInit, i, Task_Flag_Kernel);
    List_del(&Init_taskStruct.listEle);
    Task_switch_init(&Init_taskStruct, initTask[0]);
}