#ifndef __TASK_DESC_H__
#define __TASK_DESC_H__

#include "../includes/lib.h"
#include "../includes/memory.h"

#define Task_stackSize 32768

typedef struct tmpTaskMemStruct {
    PageTable *pgd;

    u64 stCode, edCode;
    u64 stData, edData;
    u64 stRodata, edRodata;
    u64 stBrk, edBrk;
    u64 stStk;
} TaskMemStruct;
typedef struct tmpThreadStruct {
    u64 rsp0;
    u64 rip;
    u64 rsp;
    u64 fs, gs;
    u64 cr2;
    u64 trapNum;
    u64 errCode;
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
    u64 stk[Task_stackSize / sizeof(u64)];
} __attribute__((aligned (8)));

typedef struct tmpTSS {
    u32 reversed0;
    u64 rsp0, rsp1, rsp2;
    u64 reversed1;
    u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    u64 reversed2;
    u16 reversed3;
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
#define Task_current Task_getCurrent()

#define Task_switchTo(prev, next) \
    do { \
        __asm__ __volatile__ ( \
            "pushq %%rbp                \n\t" /*save the stack*/ \
            "pushq %%rax                \n\t" \
            "movq %%rsp, %0             \n\t" \
            "movq %2, %%rsp             \n\t" \
            "movq 1f(%%rip), %%rax      \n\t" \
            "movq %%rax, %1             \n\t" \
            "pushq %3                   \n\t" \
            "jmpq Task_switchTo_inner   \n\t" \
            "1:                         \n\t" \
            "popq %%rax                 \n\t" \
            "popq %%rbp                 \n\t" \
            : "=m"((prev)->thread->rsp), "=m"((next)->thread->rip) \
            : "m"((next)->thread->rsp), "m"((next)->thread->rip), "D"(prev), "S"(next) \
            : "memory" \
        ); \
    } while (0)

void Init_task();

#endif