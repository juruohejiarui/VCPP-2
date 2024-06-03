#include "mgr.h"
#include "../includes/log.h"

extern void Task_kernelThreadEntry();
extern void restoreAll();

int Task_pidCounter;

void Task_checkPtRegInStack(u64 rsp) {
    printk(WHITE, BLACK, "rsp: %#018lx, cr3 = %#018lx, rflags = %#018lx\n", rsp, getCR3(), IO_getRflags());
    for (int i = 0; i < sizeof(PtReg) / sizeof(u64); i++)
        printk(WHITE, BLACK, "rsp+%#04x: %#018lx%c", i * 8, *(u64 *)(rsp + i * 8), (i + 1) % 8 == 0 ? '\n' : ' ');
}

#define Task_initTask(task) \
{ \
    .state      = Task_State_Uninterruptible, \
    .flags      = Task_Flag_Kernel, \
    .mem        = &Init_mm, \
    .thread     = &Init_thread, \
    .pid        = 0, \
    .vRunTime   = 1, \
    .signal     = 0, \
    .priority   = 0 \
}

#define Task_initTSS(intrStackEnd) \
{ \
    .reserved0 = 0, \
    .rsp0 = Task_kernelStackEnd, \
    .rsp1 = Task_kernelStackEnd, \
    .rsp2 = Task_kernelStackEnd, \
    .reserved1 = 0, \
    .ist1 = (intrStackEnd), \
    .ist2 = (intrStackEnd), \
    .ist3 = (intrStackEnd), \
    .ist4 = (intrStackEnd), \
    .ist5 = (intrStackEnd), \
    .ist6 = (intrStackEnd), \
    .ist7 = (intrStackEnd), \
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

TSS Init_TSS[Hardware_CPUNumber] = { [0 ... Hardware_CPUNumber - 1] = Task_initTSS(0xffff800000007c00) };
TaskMemStruct Init_mm = {0};

TaskStruct Init_taskStruct = Task_initTask(NULL);
TaskStruct *Init_tasks[Hardware_CPUNumber] = { &Init_taskStruct, 0 };

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
    next->tss->rsp0 = next->thread->rsp0;
    if (next->thread->rip > 0xffff800000000000 + 0x3000000 || next->thread->rip < 0xffff800000000000) {
        printk(RED, BLACK, "From %#018lx, to %#018lx, invalid rip: %#018lx\n", prev, next, next->thread->rip);
        while (1) ;
    }
    Intr_Gate_setTSS(
            next->tss->rsp0, next->tss->rsp1, next->tss->rsp2, next->tss->ist1, next->tss->ist2,
			next->tss->ist3, next->tss->ist4, next->tss->ist5, next->tss->ist6, next->tss->ist7);
    __asm__ volatile ( "movq %%fs, %0 \n\t" : "=a"(prev->thread->fs));
    __asm__ volatile ( "movq %%gs, %0 \n\t" : "=a"(prev->thread->gs));
    __asm__ volatile ( "movq %0, %%fs \n\t" : : "a"(next->thread->fs));
    __asm__ volatile ( "movq %0, %%gs \n\t" : : "a"(next->thread->gs));
}

i64 _weight[50] = { 1, 2, 3, 4, 5, [5 ... 49] = -1 };

struct CFS_rq {
    TaskStruct *next[50];
    RBTree tree;
} _CFSstruct;

void Task_initMgr() {
    RBTree_init(&_CFSstruct.tree);
}

void Task_updateCurState() {
    IO_Func_maskIntrPreffix
    // update the vRunTime and then check if needs schedule
    Task_current->vRunTime += _weight[Task_current->priority];
    Task_current->state = Task_State_NeedSchedule;
    IO_Func_maskIntrSuffix
}

void Task_schedule() {
    IO_cli();
    RBNode *leftMost = RBTree_getMin(&_CFSstruct.tree);
    TaskStruct *next = container(leftMost->head.next, TaskStruct, listEle);
    RBTree_del(&_CFSstruct.tree, leftMost->val);
    TaskStruct *dmas_ptr = (TaskStruct *)DMAS_phys2Virt(MM_PageTable_getPldEntry(getCR3(), (u64)Task_current) & ~0xfff);
    RBTree_insert(&_CFSstruct.tree, Task_current->vRunTime, &dmas_ptr->listEle);
    Intr_SoftIrq_Timer_initIrq(&Task_current->timerIrq, 1, Task_updateCurState, NULL);
    Intr_SoftIrq_Timer_addIrq(&Task_current->timerIrq);
    Task_switch(next);
}

TaskStruct *Task_createTask(u64 (*kernelEntry)(u64 (*)(u64), u64), u64 (*usrEntry)(u64), u64 arg, u64 flags) {
    u64 pgdPhyAddr = MM_PageTable_alloc(); Page *tskStructPage = MM_Buddy_alloc(0, Page_Flag_Active);
    printk(WHITE, BLACK, "pgdPhyAddr: %#018lx, tskStructPage: %#018lx\n", pgdPhyAddr, tskStructPage->phyAddr);

    // copy the kernel part (except stack) of pgd
    memcpy((u64 *)DMAS_phys2Virt(getCR3()) + 256, (u64 *)DMAS_phys2Virt(pgdPhyAddr) + 256, 255 * sizeof(u64));
    // set the Task_current
    MM_PageTable_map(pgdPhyAddr, Task_kernelStackEnd - Task_kernelStackSize, tskStructPage->phyAddr, MM_PageTable_Flag_Presented | MM_PageTable_Flag_Writable);

	Page *lstPage = MM_Buddy_alloc(5, Page_Flag_Active);
	// map the interrupt stack with full present pages
	for (u64 vAddr = Task_intrStackEnd - Task_intrStackSize; vAddr < Task_intrStackEnd; vAddr += Page_4KSize)
		MM_PageTable_map(pgdPhyAddr, 
                vAddr, lstPage->phyAddr + vAddr - (Task_intrStackEnd - Task_intrStackSize), 
                MM_PageTable_Flag_Presented | MM_PageTable_Flag_Writable | MM_PageTable_Flag_UserPage);
    // map the user stack without present flag
    for (u64 vAddr = Task_userStackEnd - Task_userStackSize + 0x10; vAddr < Task_userStackEnd; vAddr += Page_4KSize)
        MM_PageTable_map(pgdPhyAddr, vAddr, 0, MM_PageTable_Flag_UserPage | MM_PageTable_Flag_Writable);
    // map the kernel stack with one present page
	lstPage = MM_Buddy_alloc(0, Page_Flag_Active);
    for (u64 vAddr = 0xFFFFFFFFFF800000 + Page_4KSize; vAddr != 0; vAddr += Page_4KSize)
        MM_PageTable_map(pgdPhyAddr,
                vAddr, vAddr == Task_kernelStackEnd - 0xff0ul ? lstPage->phyAddr : 0, 
                MM_PageTable_Flag_Writable | (vAddr == Task_kernelStackEnd - 0xff0ul ? MM_PageTable_Flag_Presented : 0));
	
    TaskStruct *task = (TaskStruct *)DMAS_phys2Virt(tskStructPage->phyAddr); 
    memset(task, 0, sizeof(TaskStruct) + sizeof(ThreadStruct) + sizeof(TaskMemStruct) + sizeof(TSS));
	
	// set the pointers of sub-structs
    ThreadStruct *thread = (ThreadStruct *)(task + 1);
	task->thread = thread;
	task->mem = (TaskMemStruct *)(thread + 1);
	task->tss = (TSS *)(task->mem + 1);
	printk(WHITE, BLACK, "task=%#018lx, mem=%#018lx, thread=%#018lx, tss=%#018lx\n", task, task->mem, task->thread, task->tss);

    task->flags = flags;
    task->vRunTime = 1;
    task->pid = Task_pidCounter++;
    task->mem->pgd = DMAS_phys2Virt(pgdPhyAddr);
    task->mem->pgdPhyAddr = pgdPhyAddr;
	task->state = Task_State_Uninterruptible;

	memset(task->tss, 0, sizeof(TSS));
    // execption stack pointer
	task->tss->ist1	= Task_intrStackEnd;
    // interrupt stack pointer
	task->tss->ist2 = Task_intrStackEnd - (Task_intrStackSize >> 1);
	task->tss->ist3 = task->tss->ist4 = task->tss->ist5 = task->tss->ist6 = task->tss->ist7 = Task_intrStackEnd;
	task->tss->rsp0 = task->tss->rsp1 = task->tss->rsp2 = Task_kernelStackEnd;

    *thread = Init_thread;
    thread->rip = (u64)Task_kernelThreadEntry;
    thread->rbp = Task_kernelStackEnd;
    thread->rsp0 = thread->rsp = Task_kernelStackEnd - sizeof(PtReg);
    thread->rsp3 = Task_userStackEnd;
	thread->fs = thread->gs = Segment_kernelData;

    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rflags = (1 << 9);
    regs.rdi = (u64)usrEntry;
	regs.rsi = arg;
    regs.cs = Segment_kernelCode;
    regs.ds = Segment_kernelData;
	regs.es = Segment_kernelData;
	regs.ss = Segment_kernelData;
	regs.rip = (u64)kernelEntry;
	regs.rsp = Task_kernelStackEnd;
    memcpy(&regs, (u64 *)DMAS_phys2Virt(lstPage->phyAddr + Page_4KSize - 16 - sizeof(PtReg)), sizeof(PtReg));

    if (task->pid > 0) RBTree_insert(&_CFSstruct.tree, 0, &task->listEle);

    RBTree_init(&task->softIrqTree);
    return task;
}

int Task_getRing() {
    u64 cs;
    __asm__ volatile ("movq %%cs, %0" : "=a"(cs));
    return cs & 3;
}
