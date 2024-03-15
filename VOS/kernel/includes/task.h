#ifndef __TASK_H__
#define __TASK_H__

#include "lib.h"
#include "memory.h"

#define STACK_SIZE 32768

#define SEGMENT_KERNEL_CS (0x08)
#define SEGMENT_KERNEL_DS (0x10)
#define SEGMENT_USER_CS (0x28)
#define SEGMENT_USER_DS (0x30)

#define TASK_STATE_RUNNING (1 << 0)
#define TASK_STATE_INTERRUPTIBLE (1 << 1)
#define TASK_STATE_UNINTERRUPTIBLE (1 << 2)
#define TASK_STATE_ZOMBIE (1 << 3)
#define TASK_STATE_STOPPED (1 << 4)

#define TASK_CLONE_FS       (1 << 0)
#define TASK_CLONE_FILES    (1 << 1)
#define TASK_CLONE_SIGNAL   (1 << 2)

#define TASK_FLAG_KTHREAD (1 << 0)

#define NR_CPUS 8

extern u64 _stack_start;

// the struct for page table info and segments of task.
typedef struct tmpTaskMemManageStruct {
    // address to the page table
    Pml4t *pgd;
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
    volatile long state;
    u64 flags;
    TaskMemManageStruct *memStruct;
    ThreadStruct *thread;

    /*user: 0x0->0x0000,7fff,ffff,ffff, kernel: 0xffff,8000,0000,0000->0xffff,ffff,ffff,ffff*/
    u64 addr;
    
    s64 pid, counter, signal, priority;
} TaskStruct;

union TaskUnion {
    TaskStruct task;
    u64 stack[STACK_SIZE / sizeof(u64)];
} __attribute__((aligned(8)));

typedef struct {
    u32 reserved0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomapBaseAddr;
} __attribute__((packed)) TSS;

extern union TaskUnion initTaskUnion;
extern TaskStruct *initTask[NR_CPUS];
extern TaskMemManageStruct initMemManageStruct;
extern ThreadStruct initThread;

void __switch_to(TaskStruct *prev, TaskStruct *next);

TaskStruct *getCurrentTask();
#define current getCurrentTask()
#define getCurrent2RBX \
    "movq %rsp, %rbx \n\t" \
    "andq $-32768, %rbx \n\t"

void Task_init();

#endif