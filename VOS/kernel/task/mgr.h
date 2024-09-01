#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/hardware.h"

struct CFS_rq {
    RBTree tree[Hardware_CPUNumber], killedTree;
	SpinLock lock[Hardware_CPUNumber], killedTreeLock;
	Atomic taskNum[Hardware_CPUNumber];
	u64 flags;
	Atomic killedTaskNum;
	Atomic recycTskState;
};
extern struct CFS_rq Task_cfsStruct;

extern TSS Init_TSS[Hardware_CPUNumber];
extern TaskStruct Init_taskStruct;

extern int Task_pidCounter;

#pragma region scheduler
void Task_updateCurState();
void Task_updateAllProcessorState();

void Task_schedule();

#define Task_switch_init(prev, next) \
	do { \
		__asm__ volatile ( \
			"movq 0x18(%%rsi), %%r8	\n\t" \
			"movq 0x20(%%rsi), %%r9	\n\t" \
			"movq 0x0(%%r8), %%rbx	\n\t" \
			"movq 0x8(%%r8), %%rdx	\n\t" \
			"movq 0x38(%%r8), %%rcx	\n\t" \
			"movq 0x0(%%r9), %%rax 	\n\t" \
			"movq %%rax, %%cr3		\n\t" \
			"mfence					\n\t" \
			"movq %%rdx, %%rsp		\n\t" \
			"pushq %%rcx			\n\t" \
			"popfq					\n\t" \
			"pushq %%rbx			\n\t" \
			"jmp Task_switchTo_inner	\n\t" \
			: \
			: "D"(prev), "S"(next) \
			: "memory" \
		); \
	} while (0)

void Task_switch(TaskStruct *next);

// the current task
#define Task_current ((TaskStruct *)(Task_kernelStackEnd - Task_kernelStackSize))

static __always_inline__ TaskStruct *Task_currentDMAS() {
	return (TaskStruct *)DMAS_phys2Virt(MM_PageTable_getPldEntry(getCR3(), (u64)Task_current) & ~0xfff);
}

void Task_exit(int retVal);

TaskStruct *Task_createTask(Task_Entry entry, void *arg1, u64 arg2, u64 flag);

void Task_recycleThread(void *arg1, u64 arg2);

#pragma region Signal
/// @brief the default signal handler for all types of signal
void Task_defaultSignalHandler(u64 signal);

/// @brief set the signal handler of current task
void Task_setSignalHandler(TaskStruct *task, u64 signal, Task_SignalHandler handler, u64 param);
/// @brief set signal to TASK
static __always_inline__ void Task_setSignal(TaskStruct *task, u64 signal) { task->signal |= (1 << signal); }
#pragma endregion

#pragma region Task Timer
// initialize a timer with JIFFIES
void Task_Timer_init(Task_Timer *timer, u64 jiffies);
/// @brief modify the timer with JIFFIES
/// @return 0: normal and modify successfully
/// @return 1: this timer has been enabled; 
/// @return -1: this timer is in timer queue and can't be modified
int Task_Timer_modJiffies(Task_Timer *timer, u64 jiffies);
/// @brief add the timer into the timer queue
/// @return 0: successfully
/// @return -1: this timer is already in timer queue and can't be added twice 
int Task_Timer_add(Task_Timer *timer);
#pragma endregion

int Task_getRing();

/// @brief the general entry for every task
extern void Task_kernelThreadEntry();

/// @brief the header for all the kernel entries
void Task_kernelEntryHeader();

/// @brief jmp to Task_exit() and wait for being recycling with a return value
/// @param retVal the return val
/// @return
static __always_inline__ void Task_kernelThreadExit(int retVal) {
	__asm__ volatile(
		// switch task to the top of kernel stack
		"leaq Task_exit(%%rip), %%rax		\n\t"
		"callq *%%rax						\n\t"
		:
		: "D"(retVal)
		: "rax", "memory"
	);
}

void Task_initMgr();
void Task_init();

#endif