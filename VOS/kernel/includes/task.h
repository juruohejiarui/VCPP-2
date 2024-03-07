#ifndef __TASK_H__
#define __TASK_H__

#include "lib.h"
#include "memory.h"

#define STACK_SIZE 32768

#define TASK_STATE_RUNNING (1 << 0)
#define TASK_STATE_INTERRUPTIBLE (1 << 1)
#define TASK_STATE_UNINTERRUPTIBLE (1 << 2)
#define TASK_STATE_ZOMBIE (1 << 3)
#define TASK_STATE_STOPPED (1 << 4)

// the struct for page table info and segments of task.
typedef struct tmpTaskMemManageStruct {
    // address to the page table
    Pml4t *pml4t;
    u64 codeSt, codeEd;
    u64 dataSt, dataEd;
    u64 rodataSt, rodataEd;
    u64 brkSt, brkEd;
    u64 stkSt;
} TaskMemManageStruct;

typedef struct tmpThreadStruct {
    u64 rsp0;
    u64 rip;
    u64 rsp;
    u64 fs;
    u64 gs;
    u64 cr2;
    u64 trapNr;
    u64 errorNode;
} ThreadStruct;

typedef struct tmpTaskStruct {
    List list;
    volatile long state, flags;
    TaskMemManageStruct memStruct;
    ThreadStruct *thread;

    /*user: 0x0->0x0000,7fff,ffff,ffff, kernel: 0xffff,8000,0000,0000->0xffff,ffff,ffff,ffff*/
    u64 addr;
    
    s64 pid, conter, signal, priority;
} TaskStruct;

union TaskUnion {
    TaskStruct task;
    u64 stack[STACK_SIZE / sizeof(u64)];
} __attribute__((aligned(8)));

#endif