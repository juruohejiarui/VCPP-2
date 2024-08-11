#ifndef __TASK_DESC_H__
#define __TASK_DESC_H__

#include "../includes/lib.h"
#include "../includes/memory.h"
#include "../includes/interrupt.h"

extern unsigned long kallsyms_addresses[] __attribute__((weak));
extern long kallsyms_syms_num __attribute__((weak));
extern long kallsyms_index[] __attribute__((weak));
extern char* kallsyms_names __attribute__((weak));

#define Init_taskStackSize 32768

#define Task_Flag_Slaver	(1 << 0)
#define Task_Flag_Kernel	(1 << 1)  
// this task use inner code of kernel
#define Task_Flag_Inner		(1 << 2)
#define Task_Flag_UseFloat  (1 << 3)

#define Task_State_Uninterruptible  (1 << 0)
#define Task_State_Running          (1 << 1)
#define Task_State_NeedSchedule     (1 << 2)
#define Task_State_Sleeping         (1 << 3)

#define Task_userStackEnd       0x00007ffffffffff0ul
#define Task_kernelStackEnd     0xfffffffffffffff0ul
#define Task_intrStackEnd	   	0xffffffffffff8000ul
#define Task_userStackSize      0x0000000001000000ul // 32M
#define Task_kernelStackSize    0x0000000000007ff0ul // 32K
#define Task_intrStackSize		0x0000000000008000ul // 32K
#define Task_userBrkStart       0x0000000000100000ul
#define Task_kernelBrkStart     0xffff800000000000ul

#define Task_Priority_Normal    0
#define Task_Priority_IO        1
#define Task_Priority_Sleeping  3
#define Task_Priority_Trapped   4
#define Task_Priority_Killed    5

typedef struct Task_KmallocUsage {
	List listEle;
	void *addr;
	void (*desctrutor)(void *);
} Task_KmallocUsage;

typedef struct TaskMemStruct {
    PageTable *pgd;
    u64 pgdPhyAddr;
    u64 totUsage;
    List pageUsage, kmallocUsage;
    Page *intrPage, *lstKerPage;
} TaskMemStruct;

typedef struct ThreadStruct {
    u64 rip;
    u64 rsp0, rsp3, rsp, rbp;
    u64 fs, gs;
    u64 cr2;
    u64 trapNum;
    u64 errCode;
    u64 rflags;
} ThreadStruct;

typedef void (*Task_SignalHandler)(u64 signal, u64 arg);
#define Task_signalNum 32
enum Task_Signal {
	Task_Signal_Int			= 1,
	Task_Signal_FloatError,
	Task_Signal_Kill
};

typedef struct Task_SIMDContext {

} Task_SIMDContext;

typedef struct TaskStruct {
    List listEle;
    volatile i64 state;	
    ThreadStruct *thread;
    TaskMemStruct *mem;
	TSS *tss;
    u64 flags;
    RBTree timerTree;
    i64 pid, vRunTime;
	u64 signal, priority;
    TimerIrq scheduleTimer;
	// the rb node for CFStree
	RBNode wNode;

	Task_SignalHandler signalHandler[Task_signalNum];
	u64 signalHandlerArg[Task_signalNum];

    Task_SIMDContext simd;
} __attribute__((packed)) TaskStruct; 

// set the signal handler of current task
void Task_setSignalHandler(u64 signal, Task_SignalHandler handler, u64 arg);
// set signal to TASK
void Task_setSignal(TaskStruct *task, u64 signal);

union TaskUnion {
    TaskStruct task;
    u64 stk[Init_taskStackSize / sizeof(u64)];
} __attribute__((aligned (8)));

void Task_init();

#endif