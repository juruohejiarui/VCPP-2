#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/hardware.h"

#define Task_switchTo(prev, next) \
    do { \
        __asm__ __volatile__ ( \
            "pushq %%rbp            \n\t" \
            "pushq %%rax            \n\t" \
            "movq %%rsp, %0         \n\t" \
            "movq %2, %%rsp         \n\t" \
            "movq 1f(%%rip), %%rax  \n\t" \
            "movq %%rax, %1         \n\t" \
            "movq %%rbx, %%cr3      \n\t" \
            "pushq %3               \n\t" \
            "jmp Task_switchTo_inner\n\t" \
            "1:                     \n\t" \
            "popq %%rax             \n\t" \
            "popq %%rbp             \n\t" \
            : "=m"((prev)->thread->rsp), "=m"((prev)->thread->rip) \
            : "m"((next)->thread->rsp), "m"((next)->thread->rip), "D"(prev), "S"(next), "b"((next)->mem->pgdPhyAddr) \
            : "memory" \
        ); \
    } while (0)

extern TSS Init_TSS[Hardware_CPUNumber];
extern TaskStruct Init_taskStruct;

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64), u64 arg, u64 flags);

TaskStruct *Task_getCurrent();
// the current task
#define Task_current Task_getCurrent()

#endif