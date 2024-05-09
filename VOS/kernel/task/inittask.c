#include "desc.h"
#include "syscall.h"
#include "mgr.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

u64 init(u64 arg) {
    // enable timer interrupt
    if (Task_current->pid == 0) APIC_enableIntr(0x14);
    printk(RED, BLACK, "init is running, arg = %#018lx\n", arg);
    // while (1) putchar(ORANGE, BLACK, arg);
    Task_switchToUsr(Task_initUsrLevel, arg);
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
    
    TaskStruct *initTask = Task_createTask(init, 10, 0), *initTask2 = Task_createTask(init, 0, 0);
    printk(ORANGE, BLACK, "initTask: %#018lx, initTask2: %#018lx\n", initTask, initTask2);
    List_del(&Init_taskStruct.listEle);
    Task_switchTo(&Init_taskStruct, initTask);
}