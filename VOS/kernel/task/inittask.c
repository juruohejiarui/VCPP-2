#include "desc.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"
#include "../includes/smp.h"

extern void Intr_retFromIntr();

extern u8 Init_stack[32768] __attribute__((__section__ (".data.Init_stack") ));

void Task_keyboardEvent(void *arg1, u64 arg2) {
	Task_kernelEntryHeader();
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
		kfree(kpEvent, 0);
	}
	while(1) IO_hlt();
	Task_kernelThreadExit(0);
}

void task_empty(void *arg1, u64 arg2) {
	Task_kernelEntryHeader();
	Task_current->priority = Task_Priority_Trapped;
	Atomic_inc(&SMP_task0LaunchNum);
	while (1) IO_hlt();
	Task_kernelThreadExit(1);
}

void init(u64 (*usrEntry)(void *, u64), u64 *argPtr) {
	Task_kernelEntryHeader();
	void *arg1 = (void *)argPtr[0];
	u64 arg2 = argPtr[1];
	kfree(argPtr, 0);
    Task_switchToUsr(usrEntry, arg1, arg2);
    Task_kernelThreadExit(0);
}

void recur(u64 arg, int dep) {
	int stk[1024];
	if (dep < 50) recur(arg, dep + 1);
	else printk(WHITE, BLACK, "user task %2d\t", arg);
}
void usrInit(void *arg1, u64 arg2) {
    // printk(WHITE, BLACK, "User level task is running, arg = %ld\n", arg);
    while (1) {
		Task_Syscall_usrAPI(2, 2000, 0, 0, 0, 0, 0);
		if (arg2 % 5 == 0) recur(arg2, 0);
		else printk(WHITE, BLACK, "user task %2d\t", arg2);
		float i = arg2 / 17.0;
		// IO_hlt();
	}
}

void task0(void *arg1, u64 arg2) {
	Task_kernelEntryHeader();
	printk(WHITE, BLACK, "task0 is running...\n");
	Atomic_inc(&SMP_task0LaunchNum);
	// launch keyboard task
	TaskStruct *kbTask = Task_createTask(Task_keyboardEvent, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel),
				*recycTask = Task_createTask(Task_recycleThread, NULL, 0, Task_Flag_Kernel | Task_Flag_Inner);
	HW_initAdvance();
	// for (int i = 0; i < 20; i++) Task_createTask(usrInit, NULL, i, Task_Flag_Inner);
	// for (int i = 0; i < 20; i++) Task_createTask(task_empty, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel);
	while (1) IO_hlt();
	Task_kernelThreadExit(0);
}

void Task_init() {
	printk(RED, BLACK, "Task_init()\n");
    Task_initMgr();
    Task_pidCounter = 0;
	// fake the task struction of the current task
    Init_taskStruct.thread->rsp = (u64)Init_stack + Init_taskStackSize;
    Init_taskStruct.thread->fs = Init_taskStruct.thread->gs = Segment_kernelData;
    List_init(&Init_taskStruct.listEle);
    
    TaskStruct **initTask = kmalloc(sizeof(TaskStruct *) * SMP_cpuNum, Slab_Flag_Clear, NULL);
	initTask[0] = Task_createTask(task0, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel);
	for (int i = 1; i < SMP_cpuNum; i++) initTask[i] = Task_createTask(task_empty, NULL, 0, Task_Flag_Inner | Task_Flag_Kernel);
    List_del(&Init_taskStruct.listEle);
	SIMD_setTS();
    Task_switch_init(&Init_taskStruct, initTask[0]);
}