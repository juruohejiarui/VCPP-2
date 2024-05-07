#include "mgr.h"
#include "../includes/log.h"

extern void Task_kernelThreadEntry();

#define Task_initTask(task) \
{ \
    .state      = Task_State_Uninterruptible, \
    .flags      = Task_Flag_Kernel, \
    .mem        = &Init_mm, \
    .thread     = &Init_thread, \
    .pid        = 0, \
    .counter    = 1, \
    .signal     = 0, \
    .priority   = 0 \
}

#define Task_initTSS() \
{ \
    .reserved0 = 0, \
    .rsp0 = Task_kernelStackEnd, \
    .rsp1 = Task_kernelStackEnd, \
    .rsp2 = Task_kernelStackEnd, \
    .reserved1 = 0, \
    .ist1 = 0xffff800000007c00, \
    .ist2 = 0xffff800000007c00, \
    .ist3 = 0xffff800000007c00, \
    .ist4 = 0xffff800000007c00, \
    .ist5 = 0xffff800000007c00, \
    .ist6 = 0xffff800000007c00, \
    .ist7 = 0xffff800000007c00, \
    .reserved2 = 0, \
    .reserved3 = 0, \
    .iomapBaseAddr = 0 \
}

const ThreadStruct Init_thread = {
    .rsp0   = (u64)(Task_kernelStackEnd),
    .rsp3   = (u64)(Task_userStackEnd),
    .rsp    = (u64)(Task_kernelStackEnd),
    .fs     = Segment_kernelData,
    .gs     = Segment_kernelData,
    .cr2    = 0,
    .trapNum= 0,
    .errCode= 0
};

TSS Init_TSS[Hardware_CPUNumber] = { [0 ... Hardware_CPUNumber - 1] = Task_initTSS() };
TaskMemStruct Init_mm = {0};

TaskStruct Init_taskStruct = Task_initTask(NULL);
TaskStruct *Init_tasks[Hardware_CPUNumber] = { &Init_taskStruct, 0 };

inline TaskStruct *Task_getCurrent() { return (TaskStruct *)(0x0000000000000000); }

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
    Init_TSS[0].rsp0 = next->thread->rsp0;
    Gate_setTSS(
            Init_TSS[0].rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
            Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7);
    __asm__ __volatile__ ( "movq %%fs, %0 \n\t" : "=a"(prev->thread->fs));
    __asm__ __volatile__ ( "movq %%gs, %0 \n\t" : "=a"(prev->thread->gs));
    __asm__ __volatile__ ( "movq %0, %%fs \n\t" : : "a"(next->thread->fs));
    __asm__ __volatile__ ( "movq %0, %%gs \n\t" : : "a"(next->thread->gs));

    printk(BLUE, BLACK, "prev->thread->rsp0: %#018lx\t", prev->thread->rsp0);
    printk(BLUE, BLACK, "next->thread->rsp0: %#018lx\n", next->thread->rsp0);
}

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64), u64 arg, u64 flags) {
    u64 pgdPhyAddr = PageTable_alloc(); Page *tskStructPage = Buddy_alloc(0, Page_Flag_Active);
    // copy the kernel part (except stack) of pgd
    memcpy((u64 *)DMAS_phys2Virt(getCR3()) + 255, (u64 *)DMAS_phys2Virt(pgdPhyAddr) + 255, 255 * sizeof(u64));
    PageTable_map(getCR3(), TASK_userBrkStart, pgdPhyAddr);
    // map the user stack without present flag
    for (u64 vAddr = Task_userStackEnd - Task_userStackSize; vAddr < Task_userStackEnd; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, 0);
    // map the kernel stack without present flag
    Page *lstPage = Buddy_alloc(0, Page_Flag_Active);
    for (u64 vAddr = Task_kernelStackEnd - Task_kernelStackSize; vAddr < Task_kernelStackEnd; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, vAddr == Task_kernelStackEnd - Page_4KSize ? lstPage->phyAddr : 0);
    TaskStruct *task = (TaskStruct *)DMAS_phys2Virt(tskStructPage->phyAddr);
    memset(task, 0, sizeof(TaskStruct) + sizeof(ThreadStruct) + sizeof(TaskMemStruct));
    ThreadStruct *thread = (ThreadStruct *)(task + 1);
    task->state = Task_State_Uninterruptible;
    task->flags = flags;
    task->mem = (TaskMemStruct *)(thread + 1);
    task->thread = thread;
    task->counter = 1;
    task->mem->pgd = DMAS_phys2Virt(pgdPhyAddr);
    task->mem->pgdPhyAddr = pgdPhyAddr;
    *task->thread = Init_thread;
    task->thread->rip = (u64)Task_kernelThreadEntry;

    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rflags = (1 << 9);
    regs.rdi = arg;
    regs.cs = Segment_kernelCode;
    regs.ds = Segment_kernelData;
    regs.rax = (u64)kernelEntry;
    memcpy(&regs, (u64 *)DMAS_phys2Virt(lstPage->phyAddr + Page_4KSize) - sizeof(PtReg), sizeof(PtReg));

    return task;
}