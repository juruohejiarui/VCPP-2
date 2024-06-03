#ifndef __TASK_DESC_H__
#define __TASK_DESC_H__

#include "../includes/lib.h"
#include "../includes/memory.h"
#include "../includes/interrupt.h"

#define Init_taskStackSize 32768

#define Thread_Flag_Main    (1 << 0)
#define Thread_Flag_Sub     (1 << 1)
#define Thread_Flag_Kernel  (1 << 2)  

#define Task_Flag_Kernel    (1 << 2)

#define Task_State_Uninterruptible  (1 << 0)
#define Task_State_Running          (1 << 1)
#define Task_State_NeedSchedule     (1 << 2)

#define Task_userStackEnd       0x00007ffffffffff0ul
#define Task_kernelStackEnd     0xfffffffffffffff0ul
#define Task_intrStackEnd	   	0xffffffffff800000ul
#define Task_userStackSize      0x0000000001000000ul // 32M
#define Task_kernelStackSize    0x00000000007ffff0ul // 8M
#define Task_intrStackSize		0x0000000000008000ul // 32K
#define Task_userBrkStart       0x0000000000100000ul
#define Task_kernelBrkStart     0xffff800000000000ul

#define Task_Priority_Normal    0
#define Task_Priority_IO        1
#define Task_Priority_Sleeping  3
#define Task_Priority_Trapped   4


typedef struct tmpTaskMemStruct {
    PageTable *pgd;
    u64 pgdPhyAddr;
} TaskMemStruct;
typedef struct tmpThreadStruct {
    u64 rip;
    u64 rsp0, rsp3, rsp, rbp;
    u64 fs, gs;
    u64 cr2;
    u64 trapNum;
    u64 errCode;
    u64 rflags;
} ThreadStruct;

typedef struct tmpTaskStruct {
    List listEle;
    volatile i64 state;
    ThreadStruct *thread;
    TaskMemStruct *mem;
	TSS *tss;
    u64 flags;
    RBTree softIrqTree;
    i64 pid, vRunTime, signal, priority;

    TimerIrq timerIrq;
} __attribute__((packed)) TaskStruct; 

union TaskUnion {
    TaskStruct task;
    u64 stk[Init_taskStackSize / sizeof(u64)];
} __attribute__((aligned (8)));

void Task_init();

#endif