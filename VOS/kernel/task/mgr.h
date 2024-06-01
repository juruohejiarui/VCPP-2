#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/hardware.h"

// #define Task_switch_init(prev, next) \
//     do { \
//         __asm__ volatile ( \
//             "movq %%rbp, %2         \n\t" \
//             "pushq %%rbp            \n\t" \
//             "pushq %%rax            \n\t" \
//             "movq %%rsp, %0         \n\t" \
//             "movq %3, %%rsp         \n\t" \
//             "movq 1f(%%rip), %%rax  \n\t" \
//             "movq %%rax, %1         \n\t" \
//             "movq %%rbx, %%cr3      \n\t" \
//             "movq %5, %%rbp         \n\t" \
//             "pushq %4               \n\t" \
//             "jmp Task_switchTo_inner\n\t" \
//             "1:                     \n\t" \
//             "popq %%rax             \n\t" \
//             "popq %%rbp             \n\t" \
//             : "=m"((prev)->thread->rsp), "=m"((prev)->thread->rip), "=m"((prev)->thread->rbp) \
//             : "m"((next)->thread->rsp), "m"((next)->thread->rip), "m"((next)->thread->rbp), "D"(prev), "S"(next)  \
//             : "memory" \
//         ); \
//     } while (0)

#define Task_switch_init(prev, next) \
	do { \
		__asm__ volatile ( \
			/* save state of the prev task */ \
			"pushq %%rbp		 	\n\t" \
			"movq 0x18(%%rdi), %%r8	\n\t" \
			"movq %%rsp, 0x18(%%r8)	\n\t" \
			"leaq 1f(%%rip), %%rax	\n\t" \
			"movq %%rax, 0x0(%%r8)	\n\t" \
			"pushfq					\n\t" \
			/* load the state of the next task */ \
			"popq 0x50(%%r8)		\n\t" \
			"movq 0x18(%%rsi), %%r8	\n\t" \
			"movq 0x20(%%rsi), %%r9	\n\t" \
			"movq 0x0(%%r8), %%rbx	\n\t" \
			"movq 0x18(%%r8), %%rdx	\n\t" \
			"movq 0x50(%%r8), %%rcx	\n\t" \
			"movq 0x8(%%r9), %%rax 	\n\t" \
			"movq %%rax, %%cr3		\n\t" \
			"mfence					\n\t" \
			"movq %%rdx, %%rsp		\n\t" \
			"pushq %%rcx			\n\t" \
			"popfq					\n\t" \
			"pushq %%rbx			\n\t" \
			"jmp Task_switchTo_inner	\n\t" \
			"1: 					\n\t" \
			"popq %%rbp				\n\t" \
			: \
			: "D"(prev), "S"(next) \
			: "memory" \
		); \
	} while (0)

void Task_checkPtRegInStack(u64 rsp);

extern TSS Init_TSS[Hardware_CPUNumber];
extern TaskStruct Init_taskStruct;

extern int Task_pidCounter;

void Task_switch(TaskStruct *to);

void Task_initMgr();

void Task_updateCurState();

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64 (*)(u64), u64), u64 (*usrEntry)(u64), u64 arg, u64 flags);

#define Task_countDown() ((--Task_current->counter) == 0)

int Task_getRing();



// the current task
#define Task_current ((TaskStruct *)(Task_userBrkStart))

#endif