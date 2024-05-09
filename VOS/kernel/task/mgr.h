#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/hardware.h"

void Task_switchTask();

#define Task_switchTo(prev, next) \
    do { \
        __asm__ __volatile__ ( \
            /* save registers */ \
            "movq %%rsp, %0         \n\t" \
            "pushfq                 \n\t" \
            "popq %2                \n\t" \
            "movq 1f(%%rip), %%rax  \n\t" \
            "movq %%rax, %1         \n\t" \
            : "=m"((prev)->thread->rsp), "=m"((prev)->thread->rip), "=m"((prev)->thread->rflags) \
            : \
            : "memory" \
        ); \
        __asm__ __volatile__ ( \
            "pushq %0               \n\t" \
            "popfq                  \n\t" \
            : \
            : "a"((next)->thread->rflags) \
            : "memory" \
        ); \
        Task_switchTo_inner((prev), (next)); \
        __asm__ __volatile__ ( \
            /* switch rsp, cr3 and rip */ \
            "movq %0, %%rsp         \n\t" \
            "movq %2, %%rax         \n\t" \
            "movq %%rax, %%cr3      \n\t" \
            "pushq %1               \n\t" \
            "retq                   \n\t" \
            "1:                     \n\t" \
            : \
            : "m"((next)->thread->rsp), "m"((next)->thread->rip), "m"((next)->mem->pgdPhyAddr)\
            : "memory" \
        ); \
    } while (0) 

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next);

extern TSS Init_TSS[Hardware_CPUNumber];
extern TaskStruct Init_taskStruct;

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64), u64 arg, u64 flags);

int Task_countDown();

TaskStruct *Task_getCurrent();
// the current task
#define Task_current Task_getCurrent()

#endif