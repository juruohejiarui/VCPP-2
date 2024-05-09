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

ThreadStruct Init_thread = {
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

inline TaskStruct *Task_getCurrent() { return (TaskStruct *)(Task_userBrkStart); }

u64 Task_pidCounter = 0;

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
    printk(WHITE, BLACK, "switch from %#018lx -> %#018lx, which rflag is %ld, rip = %#018lx\n", prev, next, next->thread->rflags, next->thread->rip);
    __asm__ __volatile__ ( "movq %%fs, %0 \n\t" : "=a"(prev->thread->fs));
    __asm__ __volatile__ ( "movq %%gs, %0 \n\t" : "=a"(prev->thread->gs));

    Init_TSS[0].rsp0 = next->thread->rsp0;
    Gate_setTSS(
            Init_TSS[0].rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
            Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7);
    
    __asm__ __volatile__ ( "movq %0, %%fs \n\t" : : "a"(next->thread->fs));
    __asm__ __volatile__ ( "movq %0, %%gs \n\t" : : "a"(next->thread->gs));
}

__asm__ (
    ".global Task_switchTask \n\t"
    "Task_switchTask: \n\t"
    "pushq %rax             \n\t"
    "movq $0x0000000000100000, %rax \n\t"
    "movq 0x18(%rax), %rax  \n\t"
    "movq (%rax), %rax      \n\t"
    "xchgq %rax, (%rsp)     \n\t"
    "subq $0x38, %rsp       \n\t"
    "pushq %rax             \n\t"
    "movq %es, %rax         \n\t"
    "pushq %rax             \n\t"
    "movq %ds, %rax         \n\t"
    "pushq %rax             \n\t"
    "pushq %rbp             \n\t"
    "pushq %rdi             \n\t"
    "pushq %rsi             \n\t"
    "pushq %rdx             \n\t"
    "pushq %rcx             \n\t"
    "pushq %rbx             \n\t"
    "pushq %r8              \n\t"
    "pushq %r9              \n\t"
    "pushq %r10             \n\t"
    "pushq %r11             \n\t"
    "pushq %r12             \n\t"
    "pushq %r13             \n\t"
    "pushq %r14             \n\t"
    "pushq %r15             \n\t"
    "movq 1f(%rip), %rax    \n\t"
    "jmp Task_switchTask_inner \n\t"
    "1:                     \n\t"
    "popq %r15              \n\t"
    "popq %r14              \n\t"
    "popq %r13              \n\t"
    "popq %r12              \n\t"
    "popq %r11              \n\t"
    "popq %r10              \n\t"
    "popq %r9               \n\t"
    "popq %r8               \n\t"
    "popq %rbx              \n\t"
    "popq %rcx              \n\t"
    "popq %rdx              \n\t"
    "popq %rsi              \n\t"
    "popq %rdi              \n\t"
    "popq %rbp              \n\t"
    "popq %rax              \n\t"
    "movq %rax, %ds         \n\t"
    "popq %rax              \n\t"
    "movq %rax, %es         \n\t"
    "popq %rax              \n\t"
    "addq $0x38, %rsp       \n\t"
    "retq                   \n\t"
);
void Task_switchTask_inner() {
    TaskStruct *next = container(Task_current->listEle.next, TaskStruct, listEle);
    Task_switchTo(Task_current, next);
}

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64), u64 arg, u64 flags)
{
    u64 pgdPhyAddr = PageTable_alloc(); Page *tskStructPage = Buddy_alloc(0, Page_Flag_Active);
    printk(WHITE, BLACK, "pgdPhyAddr: %#018lx, tskStructPage: %#018lx\n", pgdPhyAddr, tskStructPage->phyAddr);
    // copy the kernel part (except stack) of pgd
    memcpy((u64 *)DMAS_phys2Virt(getCR3()) + 256, (u64 *)DMAS_phys2Virt(pgdPhyAddr) + 256, 255 * sizeof(u64));
    PageTable_map(pgdPhyAddr, Task_userBrkStart, tskStructPage->phyAddr);
    // map the user stack without present flag
    for (u64 vAddr = Task_userStackEnd - Task_userStackSize; vAddr < Task_userStackEnd; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, 0);
    // map the kernel stack without present flag
    Page *lstPage = Buddy_alloc(0, Page_Flag_Active);
    for (u64 vAddr = 0xFFFFFFFFFF800000; vAddr != 0; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, vAddr == 0xFFFFFFFFFFFFF000 ? lstPage->phyAddr : 0);
    TaskStruct *task = (TaskStruct *)DMAS_phys2Virt(tskStructPage->phyAddr);
    memset(task, 0, sizeof(TaskStruct) + sizeof(ThreadStruct) + sizeof(TaskMemStruct));
    ThreadStruct *thread = (ThreadStruct *)(task + 1);
    task->state = Task_State_Uninterruptible;
    task->flags = flags;
    task->mem = (TaskMemStruct *)(thread + 1);
    task->thread = thread;
    task->counter = 10;
    task->pid = Task_pidCounter++;
    task->mem->pgd = DMAS_phys2Virt(pgdPhyAddr);
    task->mem->pgdPhyAddr = pgdPhyAddr;
    *thread = Init_thread;
    thread->rip = (u64)Task_kernelThreadEntry;
    thread->rbp = Task_kernelStackEnd;
    thread->rsp0 = thread->rsp = Task_kernelStackEnd - sizeof(PtReg);
    thread->rsp3 = Task_userStackEnd - sizeof(PtReg);
    thread->rflags = (1 << 9);

    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rflags = (1 << 9);
    regs.rdi = arg;
    regs.cs = Segment_kernelCode;
    regs.ds = Segment_kernelData;
    regs.rbx = (u64)kernelEntry;
    memcpy(&regs, (u64 *)DMAS_phys2Virt(lstPage->phyAddr + Page_4KSize - 16 - sizeof(PtReg)), sizeof(PtReg));

    List_init(&task->listEle);
    List_insBehind(&task->listEle, &Init_taskStruct.listEle);
    return task;
}

u64 Task_tick;
int Task_countDown() {
    Task_current->counter--;
    if (Task_current->counter == 0) {
        Task_current->counter = 10;
        return 1;
    } else return 0;
}
