#include "desc.h"
#include "syscall.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

u64 init(u64 arg) {
    printk(RED, BLACK, "init is running, arg = %#018lx\n", arg);
    Task_switchToUsr(Task_initUsrLevel, 114514);
    return 1;
}

u64 Task_doExit(u64 arg) {
    printk(RED, BLACK, "Task_doExit is running, arg = %#018lx\n", arg);
    while (1);
}

void Init_task() {
    // fake the task struction of the current task
    Init_taskStruct.thread->rsp0 = Init_taskStruct.thread->rsp = 0xffff800000007E00;
    Init_taskStruct.thread->fs = Init_taskStruct.thread->gs = Segment_kernelData;
    List_init(&Init_taskStruct.listEle);
    
    TaskStruct *initTask = Task_createTask(init, 10, 0);
    Task_switchTo(&Init_taskStruct, initTask);
}