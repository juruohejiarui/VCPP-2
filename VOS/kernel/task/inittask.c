#include "desc.h"
#include "syscall.h"
#include "../includes/hardware.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

extern void Intr_retFromIntr();

u64 init(u64 arg) {
    printk(RED, BLACK, "init is running, arg = %#018lx\n", arg);
    Task_switchToUsr(Task_initUsrLevel, 114514);
    return 1;
}

u64 Task_doExit(u64 arg) {
    printk(RED, BLACK, "Task_doExit is running, arg = %#018lx\n", arg);
    while (1);
}

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
    .rsp0 = (u64)(Init_taskUnion.stk + Init_taskStackSize / sizeof(u64)), \
    .rsp1 = (u64)(Init_taskUnion.stk + Init_taskStackSize / sizeof(u64)), \
    .rsp2 = (u64)(Init_taskUnion.stk + Init_taskStackSize / sizeof(u64)), \
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

extern ThreadStruct Init_thread;

TaskMemStruct Init_mm = {0};

union TaskUnion Init_taskUnion __attribute__((__section__(".data.Init_taskUnion"))) = { Task_initTask(Init_taskUnion.task) };

TaskStruct *Init_tasks[Hardware_CPUNumber] = { &Init_taskUnion.task, 0 };

ThreadStruct Init_thread = {
    .rsp0   = (u64)(Init_taskUnion.stk + Init_taskStackSize / sizeof(u64)),
    .rsp    = (u64)(Init_taskUnion.stk + Init_taskStackSize / sizeof(u64)),
    .fs     = Segment_kernelData,
    .gs     = Segment_kernelData,
    .cr2    = 0,
    .trapNum= 0,
    .errCode= 0
};

TSS Init_TSS[Hardware_CPUNumber] = { [0 ... Hardware_CPUNumber - 1] = Task_initTSS() };

extern u64 Init_stack;

TaskStruct *Task_getCurrent() {
    TaskStruct *current;
    __asm__ __volatile__("andq %%rsp, %0" : "=r"(current) : "0"(~32767ul));
    return current;
}

#define Task_switchTo(prev, next) \
    do { \
        __asm__ __volatile__ ( \
            "pushq %%rbp                \n\t" /*save the stack*/ \
            "pushq %%rax                \n\t" \
            "movq %%rsp, %0             \n\t" /*save the rip and rsp of the prev thread*/ \
            "movq %2, %%rsp             \n\t" \
            "movq 1f(%%rip), %%rax      \n\t" /*load the rip and rsp of the next thread*/ \
            "movq %%rax, %1             \n\t" \
            "pushq %3                   \n\t" \
            "jmp Task_switchTo_inner   \n\t" /*call inner function for switch tss structure and segment registers*/ \
            "1:                         \n\t" \
            "popq %%rax                 \n\t" \
            "popq %%rbp                 \n\t" \
            : "=m"((prev)->thread->rsp), "=m"((prev)->thread->rip) \
            : "m"((next)->thread->rsp), "m"((next)->thread->rip), "D"(prev), "S"(next) \
            : "memory" \
        ); \
    } while (0)

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

u64 Task_doFork(PtReg *regs, u64 cloneFlags, u64 stackBase, u64 stackSize) {
    TaskStruct *tsk = NULL;
    ThreadStruct *thread = NULL;
    Page *page = NULL;
    page = Buddy_alloc(1, Page_Flag_Kernel | Page_Flag_Active);
    tsk = (TaskStruct *)DMAS_phys2Virt(page->phyAddr);
    printk(WHITE, BLACK, "TaskStruct Address: %#018lx\n", (u64)tsk);
    memset(tsk, 0, sizeof(*tsk));
    *tsk = *Task_current;
    printk(BLUE, BLACK, "finished copying task struct\n");

    List_init(&tsk->listEle);
    List_insBehind(&Init_taskUnion.task.listEle, &tsk->listEle);
    tsk->pid++;
    tsk->state = Task_State_Uninterruptible;
    thread = (ThreadStruct *)(tsk + 1);
    tsk->thread = thread;
    // copy the registers into the task of the thread
    memcpy(regs, (void *)((u64)tsk + Init_taskStackSize - sizeof(PtReg)), sizeof(PtReg));

    thread->rsp0 = (u64)tsk + Init_taskStackSize;
    thread->rip = regs->rip;
    thread->rsp = (u64)tsk + Init_taskStackSize - sizeof(PtReg);

    if (!(tsk->flags & Task_Flag_Kernel))
        thread->rip = regs->rip = (u64)Intr_retFromIntr;
    tsk->state = Task_State_Running;
    printk(BLUE, BLACK, "finished Task_doFork()\n");
    return 0;
}

extern void Task_kernelThreadEntry();

int Task_createKernelThread(u64 (*func)(u64), u64 arg, u64 flags) {
    PtReg regs;
    memset(&regs, 0, sizeof(regs));
    regs.rbx = (u64)func;
    regs.rdx = (u64)arg;
    regs.ds = Segment_kernelData;
    regs.es = Segment_kernelData;
    regs.cs = Segment_kernelCode;
    regs.ss = Segment_kernelData;
    regs.rflags = (1 << 9);
    regs.rip = (u64)Task_kernelThreadEntry;

    return Task_doFork(&regs, flags, 0, 0);
}

void Init_initTask() {
    TaskStruct *p = NULL;
    Init_mm.pgd = (PageTable *)DMAS_phys2Virt(getCR3());
    Init_mm.stCode = (u64)&_text;
    Init_mm.edCode = (u64)&_etext;
    Init_mm.stData = (u64)&_data;
    Init_mm.edData = (u64)&_edata;
    Init_mm.stRodata = (u64)&_rodata;
    Init_mm.edRodata = (u64)&_erodata;
    Init_mm.stBrk = 0;
    Init_mm.edBrk = memManageStruct.edOfStruct;
    Init_mm.stStk = Init_stack;

    Gate_setTSS(
        Init_thread.rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
        Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7
    );
    List_init(&Init_taskUnion.task.listEle);

    Task_createKernelThread(init, 10, 0);
    Init_taskUnion.task.state = Task_State_Running;
    p = container(Task_current->listEle.next, TaskStruct, listEle);
    Task_switchTo(Task_current, p);
}

void Init_task() {
    Init_initTask();
}