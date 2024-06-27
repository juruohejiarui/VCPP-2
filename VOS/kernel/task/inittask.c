#include "desc.h"
#include "syscall.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

extern volatile int Global_state;

u64 Task_keyboardEvent(u64 (*usrEntry)(u64), u64 arg) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	printk(WHITE, BLACK, "Keyboard Event monitor is running...\n");
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
	while(1) IO_hlt();
}

u64 task0(u64 (*usrEntry)(u64), u64 arg) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
	printk(WHITE, BLACK, "task0 is running...\n");
	// launch keyboard task
	Global_state = 1;
	TaskStruct *kbTask = Task_createTask(Task_keyboardEvent, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel);
	for (List *list = HW_USB_XHCI_mgrList.next; list != &HW_USB_XHCI_mgrList; list = list->next)
		Task_createTask(HW_USB_XHCI_thread, NULL, (u64)container(list, USB_XHCIController, listEle), Task_Flag_Inner | Task_Flag_Kernel);
	while (1) IO_hlt();
}

u64 init(u64 (*usrEntry)(u64), u64 arg) {
	Intr_SoftIrq_Timer_initIrq(&Task_current->scheduleTimer, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->scheduleTimer);
	Task_current->state = Task_State_Running;
    Task_switchToUsr(usrEntry, Task_current->pid << 32 | arg);
    return 1;
}

u64 usrInit(u64 arg) {
	arg >>= 32;
    printk(WHITE, BLACK, "User level task is running, arg = %ld\n", arg);
    u64 res = Task_Syscall_usrAPI(arg, BLACK, WHITE, (u64)"Up Down Up Down baba", 20, 5);
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    while (1) {
		Task_Syscall_usrAPI(3, 1000, 0, 0, 0, 0);
		Task_Syscall_usrAPI(1, BLACK, WHITE, (u64)"User Task[doge]", 16, 5);
	}
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
	initTask[0] = Task_createTask(task0, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel);
    for (int i = 1; i < 3; i++)
        initTask[i] = Task_createTask(init, usrInit, i, Task_Flag_Inner);
    List_del(&Init_taskStruct.listEle);
    Task_switch_init(&Init_taskStruct, initTask[0]);
}