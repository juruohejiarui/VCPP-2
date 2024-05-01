#ifndef __TASK_DESC_H__
#define __TASK_DESC_H__

#include "../includes/lib.h"
#include "../includes/memory.h"

#define Init_taskStackSize 32768

#define Thread_Flag_Main      (1 << 0)
#define Thread_Flag_Sub       (1 << 1)
#define Task_Flag_Kernel    (1 << 2)

#define Task_State_Uninterruptible  (1 << 0)
#define Task_State_Running          (1 << 0)


typedef struct tmpTaskMemStruct {
    PageTable *pgd;

    u64 stCode, edCode;
    u64 stData, edData;
    u64 stRodata, edRodata;
    u64 stBrk, edBrk;
    u64 stStk;
} TaskMemStruct;
typedef struct tmpThreadStruct {
    List listEle;
    u64 rsp0; // the base of the stack
    u64 rip;
    u64 rsp;
    u64 fs, gs;
    u64 cr2;
    u64 trapNum;
    u64 errCode;
    u64 flags;
} ThreadStruct;

typedef struct tmpTaskStruct {
    List listEle;
    volatile i64 state;
    ThreadStruct *thread;
    TaskMemStruct *mem;
    u64 flags;
    i64 pid, counter, signal, priority;
} TaskStruct; 

union TaskUnion {
    TaskStruct task;
    u64 stk[Init_taskStackSize / sizeof(u64)];
} __attribute__((aligned (8)));

typedef struct tmpTSS {
    u32 reserved0;
    u64 rsp0, rsp1, rsp2;
    u64 reserved1;
    u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomapBaseAddr;
} __attribute__((packed)) TSS;

typedef struct tmpPtReg {
    u64 r15, r14, r13, r12, r11, r10, r9, r8;
    u64 rbx, rcx, rdx, rsi, rdi, rbp;
    u64 ds, es;
    u64 rax;
    u64 func, errCode;
    u64 rip, cs, rflags, rsp, ss;
} PtReg;

TaskStruct *Task_getCurrent();
// the current task
#define Task_current Task_getCurrent()

void Init_task();

#endif