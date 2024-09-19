#ifndef __TASK_DESC_H__
#define __TASK_DESC_H__

#include "../includes/lib.h"
#include "../includes/memory.h"
#include "../includes/interrupt.h"
#include "../includes/simd.h"

extern unsigned long kallsyms_addresses[] __attribute__((weak));
extern long kallsyms_syms_num __attribute__((weak));
extern long kallsyms_index[] __attribute__((weak));
extern char* kallsyms_names __attribute__((weak));

#define Init_taskStackSize 32768

#define Task_Flag_Slaver		(1 << 0)
#define Task_Flag_Kernel		(1 << 1)  
// this task use inner code of kernel
#define Task_Flag_Inner			(1 << 2)
#define Task_Flag_UseFloat  	(1 << 3)
#define Task_Flag_InKillTree	(1 << 4)


#define Task_State_Uninterruptible  (1 << 0)
#define Task_State_Running          (1 << 1)
#define Task_State_NeedSchedule     (1 << 2)
#define Task_State_Sleeping         (1 << 3)

#define Task_userStackEnd       0x00007ffffffffff0ul
#define Task_kernelStackEnd     0xfffffffffffffff0ul
#define Task_intrStackEnd	   	0xffffffffffff8000ul
#define Task_userStackSize      0x0000000000fffff0ul // 32M
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
	u64 size;
	void *addr;
	void (*destructor)(void *);
} Task_KmallocUsage;

typedef struct TaskMemStruct {
    u64 pgdPhyAddr;
    u64 totUsage;
    List pageUsage, kmallocUsage;
    Page *intrPage, *tskPage;
} TaskMemStruct;

typedef struct ThreadStruct {
    u64 rip;
    u64 rsp;
    u64 fs, gs;
    u64 cr2;
    u64 trapNum;
    u64 errCode;
    u64 rflags;
} ThreadStruct;

typedef void (*Task_SignalHandler)(u64 signal, u64 param);

#define Task_signalNum 64
enum Task_Signal {
	Task_Signal_Int			= 0,
	Task_Signal_Kill,
	Task_Signal_FloatError	= 32,
	Task_Signal_Timer,
};

#define Task_Timer_Flag_Enabled		(1 << 0)
#define Task_Timer_Flag_Expire		(1 << 1)
#define Task_Timer_Flag_InQueue		(1 << 2)

typedef struct Task_Timer {
	void (*func)(u64);
	u64 data;
	u64 flags;
	TimerIrq irq;
	RBNode wNode;
} Task_Timer;

typedef struct TaskStruct {
    List listEle;
    volatile i64 state;	
    ThreadStruct *thread;
    TaskMemStruct *mem;
	TSS *tss;
    u64 flags;
    i64 vRunTime;
	i32 pid, cpuId;
	u64 signal, priority;

	RBTree timerTree;
	SpinLock timerTreeLock;

	// the rb node for CFStree
	RBNode wNode;

	Task_SignalHandler signalHandler[Task_signalNum];
	u64 signalHandlerParam[Task_signalNum];

    SIMD_XsaveArea *simdRegs;
} __attribute__((packed)) TaskStruct; 

union TaskUnion {
    TaskStruct task;
    u64 stk[Init_taskStackSize / sizeof(u64)];
} __attribute__((aligned (8)));

typedef void (*Task_Entry)(void *arg1, u64 arg2);
#endif