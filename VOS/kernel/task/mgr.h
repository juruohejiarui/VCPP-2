#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/hardware.h"

#define Task_switch_init(prev, next) \
	do { \
		__asm__ volatile ( \
			/* save state of the prev task */ \
			"cmpq $0, %%rdi			\n\t" \
			"je 2f					\n\t" \
			"pushq %%rbp		 	\n\t" \
			"movq 0x18(%%rdi), %%r8	\n\t" \
			"movq %%rsp, 0x18(%%r8)	\n\t" \
			"leaq 1f(%%rip), %%rax	\n\t" \
			"movq %%rax, 0x0(%%r8)	\n\t" \
			"pushfq					\n\t" \
			"2:						\n\t" \
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

struct CFS_rq {
    RBTree tree[Hardware_CPUNumber], killedTree;
	SpinLock lock[Hardware_CPUNumber], killedTreeLock;
    // which task domain the SIMD registers of the specific CPU
};
extern struct CFS_rq Task_cfsStruct;

void Task_updateCurState();

extern TSS Init_TSS[Hardware_CPUNumber];
extern TaskStruct Init_taskStruct;

extern int Task_pidCounter;

void Task_switch(TaskStruct *next);

// the current task
#define Task_current ((TaskStruct *)(Task_kernelStackEnd - Task_kernelStackSize))

TaskStruct *Task_currentDMAS();

void Task_initMgr();

void Task_exit();

void Task_schedule();

void Task_defaultSignalHandler(u64 signal);

u64 Task_recycleThread(u64 (*usrEntry)(u64), u64 arg);

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64 (*)(u64), u64), u64 (*usrEntry)(u64), u64 arg, u64 flag);

#define Task_countDown() ((--Task_current->counter) == 0)

int Task_getRing();

int Task_sleep();

void Task_stopSleep();

void Task_kernelEntryHeader();

static __always_inline__ void Task_kernelThreadExit(int retVal) {
	__asm__ volatile(
		// switch task to intr Task
		"movq $0xffffffffffff8000, %%rsp	\n\t"
		"movq %0, %%rax						\n\t"
		"leaq Task_exit(%%rip), %%rbx		\n\t"
		"callq *%%rbx						\n\t"
		: "=m"(retVal)
		:
		: "rax", "rbx", "memory"
	);
}

#endif