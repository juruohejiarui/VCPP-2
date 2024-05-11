#include "mgr.h"
#include "../includes/log.h"

extern void Task_kernelThreadEntry();

void Task_checkPtRegInStack(u64 rsp) {
	u64 rflags = 0;
	__asm__ volatile (
		"pushfq		\n\t"
		"popq %0	\n\t"
		: "=m"(rflags)
		:
		: "memory");
    printk(WHITE, BLACK, "rsp: %#018lx, cr3 = %#018lx, rflags = %#018lx\n", rsp, getCR3(), rflags);
    for (int i = 0; i < sizeof(PtReg) / sizeof(u64); i++)
        printk(WHITE, BLACK, "rsp + %#04x: %#018lx%c", i * 8, *(u64 *)(rsp + i * 8), (i + 1) % 8 == 0 ? '\n' : ' ');
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

__asm__ (
	".global Task_switch		\n\t"
	"Task_switch:				\n\t"
	"movq $0x100000, %rdi		\n\t"
	"movq 0x8(%rdi), %rsi		\n\t"
	"pushq %rsi					\n\t"
	"pushq %rdi					\n\t"
	"call Task_switchTo_inner	\n\t"
	"popq %rdi					\n\t"
	"popq %rsi					\n\t"
	"movq 0x20(%rsi), %r8		\n\t"
	"movq 0x8(%r8), %rax		\n\t"
	"movq %rax, %cr3			\n\t"
	"mfence						\n\t"
	"movq 0x18(%rdi), %rax		\n\t"
	"movq 0x18(%rax), %rsp		\n\t"
    "pushq 0x50(%rax)           \n\t"
    "popfq                      \n\t"
	"movq 0x0(%rax), %rax		\n\t"
	"jmp *%rax					\n\t"
);

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
    Init_TSS[0].rsp0 = next->thread->rsp0;
    Gate_setTSS(
            Init_TSS[0].rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
            Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7);
    __asm__ volatile ( "movq %%fs, %0 \n\t" : "=a"(prev->thread->fs));
    __asm__ volatile ( "movq %%gs, %0 \n\t" : "=a"(prev->thread->gs));
    __asm__ volatile ( "movq %0, %%fs \n\t" : : "a"(next->thread->fs));
    __asm__ volatile ( "movq %0, %%gs \n\t" : : "a"(next->thread->gs));
    // printk(BLUE, BLACK, "prev: %#018lx, prev->thread->rsp: %#018lx\t", prev, prev->thread->rsp);
    // printk(BLUE, BLACK, "next: %#018lx, next->thread->rsp: %#018lx\n", next, next->thread->rsp);
}

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64), u64 arg, u64 flags) {
    static int Task_pidCounter = 0;
    u64 pgdPhyAddr = PageTable_alloc(); Page *tskStructPage = Buddy_alloc(0, Page_Flag_Active);
    printk(WHITE, BLACK, "pgdPhyAddr: %#018lx, tskStructPage: %#018lx\n", pgdPhyAddr, tskStructPage->phyAddr);
    // copy the kernel part (except stack) of pgd
    memcpy((u64 *)DMAS_phys2Virt(getCR3()) + 256, (u64 *)DMAS_phys2Virt(pgdPhyAddr) + 256, 255 * sizeof(u64));
    PageTable_map(pgdPhyAddr, Task_userBrkStart, tskStructPage->phyAddr);
	Page *lstPage = Buddy_alloc(0, Page_Flag_Active);
    // map the user stack without present flag
    for (u64 vAddr = Task_userStackEnd - Task_userStackSize; vAddr < Task_userStackEnd; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, 0);
    // map the kernel stack without present flag
    // lstPage = Buddy_alloc(0, Page_Flag_Active);
    for (u64 vAddr = 0xFFFFFFFFFF800000; vAddr != 0; vAddr += Page_4KSize)
        PageTable_map(pgdPhyAddr, vAddr, vAddr == 0xFFFFFFFFFFFFF000 ? lstPage->phyAddr : 0);
    TaskStruct *task = (TaskStruct *)DMAS_phys2Virt(tskStructPage->phyAddr);
    memset(task, 0, sizeof(TaskStruct) + sizeof(ThreadStruct) + sizeof(TaskMemStruct));
    ThreadStruct *thread = (ThreadStruct *)(task + 1);
    task->flags = flags;
    task->mem = (TaskMemStruct *)(thread + 1);
    task->thread = thread;
    task->counter = 1;
    task->pid = Task_pidCounter++;
    task->mem->pgd = DMAS_phys2Virt(pgdPhyAddr);
    task->mem->pgdPhyAddr = pgdPhyAddr;
	task->state = Task_State_Uninterruptible;

    *thread = Init_thread;
    thread->rip = (u64)Task_kernelThreadEntry;
    thread->rbp = Task_kernelStackEnd;
    thread->rsp0 = thread->rsp = Task_kernelStackEnd - sizeof(PtReg);
    thread->rsp3 = Task_userStackEnd;
	thread->fs = thread->gs = Segment_kernelData;

    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rflags = (1 << 9);
    regs.rdi = arg;
    regs.cs = Segment_kernelCode;
    regs.ds = Segment_kernelData;
	regs.es = Segment_kernelData;
	regs.ss = Segment_kernelData;
	regs.rip = (u64)kernelEntry;
	regs.rsp = Task_kernelStackEnd;
    memcpy(&regs, (u64 *)DMAS_phys2Virt(lstPage->phyAddr + Page_4KSize - 16 - sizeof(PtReg)), sizeof(PtReg));

    List_init(&task->listEle);
    List_insBehind(&task->listEle, &Init_taskStruct.listEle);
    return task;
}

int Task_getRing() {
    u64 cs;
    __asm__ volatile ("movq %%cs, %0" : "=a"(cs));
    return cs & 3;
}
