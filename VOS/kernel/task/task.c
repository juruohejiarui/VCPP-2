#include "../includes/task.h"
#include "../includes/gate.h"
#include "../includes/memory.h"
#include "../includes/printk.h"

extern void ret_from_intr();

#define INIT_TASK(tsk) { \
    .state      = TASK_STATE_UNINTERRUPTIBLE, \
    .flags      = TASK_FLAG_KTHREAD, \
    .memStruct  = &initMemManageStruct, \
    .thread     = &initThread, \
    .addr       = 0xffff800000000000, \
    .pid        = 0, \
    .counter    = 1, \
    .signal     = 0, \
    .priority   = 0, \
}

#define INIT_TSS() { \
    .reserved0 = 0, \
    .rsp0 = (u64)(initTaskUnion.stack + STACK_SIZE / sizeof(u64)), \
    .rsp1 = (u64)(initTaskUnion.stack + STACK_SIZE / sizeof(u64)), \
    .rsp2 = (u64)(initTaskUnion.stack + STACK_SIZE / sizeof(u64)), \
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

union TaskUnion initTaskUnion __attribute__((__section__(".data.initTask"))) = { INIT_TASK(initTaskUnion.task) };
TaskStruct *initTask[NR_CPUS] = {&initTaskUnion.task};
TaskMemManageStruct initMemManageStruct = {0};
ThreadStruct initThread = {
    .rsp0   = (u64)(initTaskUnion.stack + STACK_SIZE / sizeof(u64)),
    .rsp    = (u64)(initTaskUnion.stack + STACK_SIZE / sizeof(u64)),
    .fs     = SEGMENT_KERNEL_DS,
    .gs     = SEGMENT_KERNEL_DS,
    .cr2    = 0,
    .trapNr = 0,
    .errorNode = 0,
};

TSS initTSS[NR_CPUS] = { [0 ... NR_CPUS - 1] = INIT_TSS() };

void __switch_to(TaskStruct *prev, TaskStruct *next);
#define switch_to(prev, next) \
    do { \
        __asm__ __volatile__( \
            "pushq %%rbp            \n\t" \
            "pushq %%rax            \n\t" \
            "movq %%rsp, %0         \n\t" \
            "movq %2, %%rsp         \n\t" \
            "leaq 1f(%%rip), %%rax  \n\t" \
            "movq %%rax, %1         \n\t" \
            "pushq %3               \n\t" \
            "jmp __switch_to        \n\t" \
            "1:                     \n\t" \
            "popq %%rax             \n\t" \
            "popq %%rbp             \n\t" \
            : "=m" ((prev)->thread->rsp), "=m" ((prev)->thread->rip) \
            : "m" ((next)->thread->rsp), "m" ((next)->thread->rip), "D" (prev), "S" (next) \
            : "memory" \
        ); \
    } while(0) 

inline void __switch_to(TaskStruct *prev, TaskStruct *next) {
    printk(WHITE, BLACK, "prev : %p, next : %p\n", prev, next);
    initTSS[0].rsp0 = next->thread->rsp0;
    setTSS64(initTSS[0].rsp0, initTSS[0].rsp1, initTSS[0].rsp2, 
        initTSS[0].ist1, initTSS[0].ist2, initTSS[0].ist3, initTSS[0].ist4, initTSS[0].ist5, initTSS[0].ist6, initTSS[0].ist7);
    __asm__ __volatile__ ("movq %%fs, %0\n\t" : "=a"(prev->thread->fs));
    __asm__ __volatile__ ("movq %%gs, %0\n\t" : "=a"(prev->thread->gs));
    __asm__ __volatile__ ("movq %0, %%fs\n\t" : : "a"(next->thread->fs));
    __asm__ __volatile__ ("movq %0, %%gs\n\t" : : "a"(next->thread->gs));

    printk(WHITE, BLACK, "prev->thread->rsp0 : %p\n", prev->thread->rsp0);
    printk(WHITE, BLACK, "next->thread->rsp0 : %p\n", next->thread->rsp0);
}

u64 init(u64 arg) {
    printk(RED, WHITE, "init task is running, arg = %#018lx\n", arg);
    while (1);
}

u64 doExit(u64 code) {
    printk(RED, BLACK, "exit code : %#018lx\n", code);
    while (1);
}

extern void kernelThreadFunc(void);
__asm__ (
    "kernelThreadFunc: \n\t"
    "   popq %r15 \n\t"
    "   popq %r14 \n\t"
    "   popq %r13 \n\t"
    "   popq %r12 \n\t"
    "   popq %r11 \n\t"
    "   popq %r10 \n\t"
    "   popq %r9 \n\t"
    "   popq %r8 \n\t"
    "   popq %rbx \n\t"
    "   popq %rcx \n\t"
    "   popq %rdx \n\t"
    "   popq %rsi \n\t"
    "   popq %rdi \n\t"
    "   popq %rbp \n\t"
    "   popq %rax \n\t"
    "   movq %rax, %ds \n\t"
    "   popq %rax \n\t"
    "   movq %rax, %es \n\t"
    "   popq %rax \n\t"
    "   addq $0x38, %rsp \n\t"
    "   movq %rdx, %rdi \n\t"
    "   call *%rbx \n\t"
    "   movq %rax, %rdi \n\t"
    "   call doExit \n\t"
);

u64 doFork(PtraceRegs *regs, u64 flag, u64 stkSt, u64 stkSize) {
    TaskStruct *task = NULL;
    ThreadStruct *thread = NULL;
    Page *p = NULL;

    printk(RED, BLACK, "before allocPages bitmap: %p\n", *memManageStruct.bitmap);

    p = allocPages(ZONE_NORMAL, 1, PAGE_PTable_Maped | PAGE_Kernel | PAGE_Active);

    printk(RED, BLACK, "after allocPages bitmap: %p\n", *memManageStruct.bitmap);
    
    task = (TaskStruct *)phyToVirt(p->phyAddr);
    printk(RED, BLACK, "task: %p current: %p\n", task, current);

    memset(task, 0, sizeof(*task));
    *task = *current;
    
    List_init(&task->list);
    List_addBefore(&initTaskUnion.task.list, &task->list);
    task->pid++;
    task->state = TASK_STATE_UNINTERRUPTIBLE;

    thread = (ThreadStruct *)(task + 1);
    task->thread = thread;
    memcpy(regs, (void *)((u64)task + STACK_SIZE - sizeof(PtraceRegs)), sizeof(PtraceRegs));
    thread->rsp0 = (u64)task + STACK_SIZE;
    thread->rip = regs->rip;
    thread->rsp = (u64)task + STACK_SIZE - sizeof(PtraceRegs);

    if (!(task->flags & TASK_FLAG_KTHREAD))
        thread->rip = regs->rip = (u64)ret_from_intr;
    
    task->state = TASK_STATE_RUNNING;

    return 0;
}

int kernelThread(u64 (*func)(u64), u64 arg, u64 flag) {
    PtraceRegs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rbx = (u64)func;
    regs.rdx = (u64)arg;

    regs.ds = SEGMENT_KERNEL_DS;
    regs.es = SEGMENT_KERNEL_DS;
    regs.cs = SEGMENT_KERNEL_CS;
    regs.ss = SEGMENT_KERNEL_DS;
    regs.rflags = (1 << 9);
    regs.rip = (u64)kernelThreadFunc;

    return doFork(&regs, flag, 0, 0);
}

void Task_init() {
    TaskStruct *p = NULL;
    initMemManageStruct.pgd = (Pml4t *)globalCR3;

    initMemManageStruct.codeSt = memManageStruct.codeSt;
    initMemManageStruct.codeEd = memManageStruct.codeEd;
    initMemManageStruct.dataSt = (u64)&_data;
    initMemManageStruct.dataEd = memManageStruct.dataEd;
    initMemManageStruct.rodataSt = (u64)&_rodata;
    initMemManageStruct.rodataEd = (u64)&_erodata;
    initMemManageStruct.brkSt = 0;
    initMemManageStruct.brkEd = memManageStruct.brkEd;
    initMemManageStruct.stkSt = _stack_start;

    setTSS64(initThread.rsp0, initTSS[0].rsp1, initTSS[0].rsp2, 
        initTSS[0].ist1, initTSS[0].ist2, initTSS[0].ist3, initTSS[0].ist4, initTSS[0].ist5, initTSS[0].ist6, initTSS[0].ist7);

    initTSS[0].rsp0 = initThread.rsp0;

    List_init(&initTaskUnion.task.list);
    printk(WHITE, BLACK, "initTaskUnion.task.list = %p, prev = %p, next = %p\n",
        &initTaskUnion.task.list,
        List_prev(&initTaskUnion.task.list),
        List_next(&initTaskUnion.task.list));
    kernelThread(init, 10, TASK_CLONE_FS | TASK_CLONE_FILES | TASK_CLONE_SIGNAL);
    initTaskUnion.task.state = TASK_STATE_RUNNING;
    printk(WHITE, BLACK, "current = %p next = %p\n", current, List_next(&current->list));
    p = container_of(List_next(&current->list), TaskStruct, list);
    printk(WHITE, BLACK, "before switch to %p\n", p);
    switch_to(current, p);
}

inline TaskStruct *getCurrentTask() {
    #ifdef current
    #undef current
    #endif
    TaskStruct *current = NULL;
    __asm__ __volatile__("andq %%rsp, %0 \n\t" : "=r" (current) : "0" (~32767UL));
    return current;
}
