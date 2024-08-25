#include "mgr.h"
#include "../includes/log.h"
#include "../includes/smp.h"
#include "desc.h"

int Task_pidCounter;

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

extern u8 Init_stack[32768];

TaskStruct Init_taskStruct = Task_initTask(NULL);
TaskStruct *Init_tasks[Hardware_CPUNumber] = { &Init_taskStruct, 0 };

#pragma region scheduler

i64 _weight[50] = { 1, 2, 3, 4, 5, 6, [6 ... 49] = -1 };

struct CFS_rq Task_cfsStruct;
TimerIrq Task_scheduleTimerIrq;

void Task_switchTo_inner(TaskStruct *prev, TaskStruct *next) {
    // set TS flag of cr0
    {
        u64 cr0 = IO_getCR(0);
        IO_setCR(0, cr0 | (1ul << 3));
    }
    SMP_CPUInfoPkg *info = SMP_current;
    next->tss->rsp0 = (next->thread->rsp & 0xffff000000000000 ? next->thread->rsp : Task_kernelStackEnd);
    // printk(RED, BLACK, "From %#018lx, to %#018lx, rip: %#018lx\n", prev, next, next->thread->rip);
    Intr_Gate_setTSS(
            info->tssTable,
            next->tss->rsp0, next->tss->rsp1, next->tss->rsp2, next->tss->ist1, next->tss->ist2,
			next->tss->ist3, next->tss->ist4, next->tss->ist5, next->tss->ist6, next->tss->ist7);
	if (prev) {
		__asm__ volatile ( "movq %%fs, %0 \n\t" : "=a"(prev->thread->fs));
    	__asm__ volatile ( "movq %%gs, %0 \n\t" : "=a"(prev->thread->gs));
	}
    __asm__ volatile ( "movq %0, %%fs \n\t" : : "a"(next->thread->fs));
    __asm__ volatile ( "movq %0, %%gs \n\t" : : "a"(next->thread->gs));
}

void Task_updateCurState() {
	Task_current->vRunTime += _weight[Task_current->priority];
	Intr_SoftIrq_setState(Task_current->cpuId, Intr_SoftIrq_State_TestSchedule);
}

void Task_SoftIrq_testSchedule() {
	Task_current->state = Task_State_NeedSchedule;
}

void Task_scheduleTimerHandler(TimerIrq *timer, void *arg) {
	// send message to all processor to test whether themselves needs schedule
	SMP_sendIPI_allButSelf(SMP_IPI_Type_Schedule, NULL);
	Task_updateCurState();
}

static int _CFSTree_comparator(RBNode *a, RBNode *b) {
	TaskStruct *task1 = container(a, TaskStruct, wNode), *task2 = container(b, TaskStruct, wNode);
	return task1->vRunTime != task2->vRunTime ? (task1->vRunTime < task2->vRunTime) : (task1->pid < task2->pid);
}

TaskStruct *Task_currentDMAS() {
	return (TaskStruct *)DMAS_phys2Virt(MM_PageTable_getPldEntry(getCR3(), (u64)Task_current) & ~0xfff);
}

void Task_schedule() {
    IO_cli();
	// printk(BLACK, WHITE, "S");
	SIMD_setTS();
	SpinLock *lock;
	RBTree *cfsTree; 
	{
		int cpuId = SMP_getCurCPUIndex();
		lock = &Task_cfsStruct.lock[cpuId];
		cfsTree = &Task_cfsStruct.tree[cpuId];
		if (cpuId == 0) {
			Intr_SoftIrq_Timer_initIrq(&Task_scheduleTimerIrq, 1, Task_scheduleTimerHandler, NULL);
			Intr_SoftIrq_Timer_addIrq(&Task_scheduleTimerIrq);
		}
	}

	SpinLock_lock(lock);
	// insert this task into the waiting tree
    if (Task_current->priority == Task_Priority_Killed && !Task_cfsStruct.recycTskState.value) {
		RBTree_insNode(&Task_cfsStruct.killedTree, &Task_currentDMAS()->wNode);
		Atomic_inc(&Task_cfsStruct.killedTaskNum);
		Task_current->flags |= Task_Flag_InKillTree;
	} else RBTree_insNode(cfsTree, &Task_currentDMAS()->wNode);
	// get the task with least vRuntime
    RBNode *leftMost = RBTree_getMin(cfsTree);

    if (leftMost == NULL) {
		// stay in the loop and wait for a task
		SpinLock_unlock(lock);
		SMP_current->flags |= SMP_CPUInfo_flag_WaitTask;
		while (1) {
			SpinLock_lock(lock);
			leftMost = RBTree_getMin(cfsTree);
			if (leftMost) break;
			SpinLock_unlock(lock);
		}
		SMP_current->flags &= ~SMP_CPUInfo_flag_WaitTask;
	}
    RBTree_delNode(cfsTree, leftMost);
	SpinLock_unlock(lock);
    TaskStruct *next = container(leftMost, TaskStruct, wNode);

    Task_switch(next);
}

#pragma endregion

#pragma region Signal

void Task_defaultSignalHandler(u64 signal) {
	printk(WHITE, BLACK, "Task %ld get signal %ld\r", Task_current->pid, signal);
	switch (signal) {
		case Task_Signal_Kill :
		case Task_Signal_Int :
		case Task_Signal_FloatError :
			Task_kernelThreadExit(-1);
			break;
		default :
			while (1) IO_hlt();
			break;
	}
}

void Task_setSignal(TaskStruct *task, u64 signal) { task->signal |= (1 << signal); }

void Task_setSignalHandler(TaskStruct *task, u64 signal, Task_SignalHandler handler) {
	task->signalHandler[signal] = handler;
}

void Task_SysSignal_Timer(u64 signal, void *arg) {
	while (1) {
		SpinLock_lock(&Task_current->timerTreeLock);
		RBNode *nd = RBTree_getMin(&Task_current->timerTree);
		Task_Timer *timer;
		if (!nd || !((timer = container(nd, Task_Timer, wNode))->flags & Task_Timer_Flag_Expire))
			break;
		RBTree_delNode(&Task_current->timerTree, nd);
		timer->flags &= ~Task_Timer_Flag_InQueue;
		SpinLock_unlock(&Task_current->timerTreeLock);
		timer->func(timer->data);
		timer->flags |= Task_Timer_Flag_Enabled;
	}
	SpinLock_unlock(&Task_current->timerTreeLock);
}

void Task_setSysSignalHandler(TaskStruct *task) {
	Task_setSignalHandler(task, Task_Signal_Timer, (Task_SignalHandler)Task_SysSignal_Timer);
}

#pragma endregion

#pragma region Task Timer

void Task_Timer_irqFunc(TimerIrq *irq, TaskStruct *tsk) {
	Task_Timer *tskTimer = container(irq, Task_Timer, irq);
	tskTimer->flags |= Task_Timer_Flag_Expire;
	Task_setSignal(tsk, Task_Signal_Timer);
}

void Task_Timer_init(Task_Timer *timer, u64 jiffies) {
	memset(timer, 0, sizeof(Task_Timer));
	Intr_SoftIrq_Timer_initIrq(&timer->irq, jiffies, (void *)Task_Timer_irqFunc, Task_currentDMAS);
}

int Task_Timer_modJiffies(Task_Timer *timer, u64 jiffies) {
	int res = 0, fl = 0;
	if (timer->flags & Task_Timer_Flag_InQueue) return -1;
	if (timer->flags & Task_Timer_Flag_Enabled) res = 1;
	Intr_SoftIrq_Timer_initIrq(&timer->irq, jiffies, (void *)Task_Timer_irqFunc, Task_currentDMAS);
	return res;
}
// add a Task Timer
int Task_Timer_add(Task_Timer *timer) {
	if (timer->flags & Task_Timer_Flag_InQueue) return -1;
	SpinLock_lock(&Task_current->timerTreeLock);
	RBTree_insNode(&Task_current->timerTree, &timer->wNode);
	SpinLock_unlock(&Task_current->timerTreeLock);
	Intr_SoftIrq_Timer_addIrq(&timer->irq);
}

int Task_Timer_comparator(RBNode *a, RBNode *b) {
	Task_Timer 	*ta = container(a, Task_Timer, wNode),
				*tb = container(b, Task_Timer, wNode);
	return (ta->irq.expireJiffies == tb->irq.expireJiffies ? (u64)ta < (u64)tb : ta->irq.expireJiffies < tb->irq.expireJiffies);
}
#pragma endregion

extern void init(u64 (*usrEntry)(void *, u64 arg), void *argPtr);

static void *_argWrap(void *arg1, u64 arg2) {
	u64 *argPkg = kmalloc(sizeof(u64) * 2, 0, NULL);
	argPkg[0] = (u64)arg1;
	argPkg[1] = arg2;
	return argPkg;
}

TaskStruct *Task_createTask(Task_Entry entry, void *arg1, u64 arg2, u64 flag) {
    u64 pgdPhyAddr = MM_PageTable_alloc(); Page *tskStructPage = MM_Buddy_alloc(5, Page_Flag_Active | Page_Flag_KernelShare);
    // printk(YELLOW, BLACK, "pgdPhyAddr: %#018lx, tskStructPage: %#018lx\t", pgdPhyAddr, tskStructPage->phyAddr);
	// contruct basic structures
	TaskStruct *task = (TaskStruct *)DMAS_phys2Virt(tskStructPage->phyAddr);
    memset(task, 0, sizeof(TaskStruct) + sizeof(ThreadStruct) + sizeof(TaskMemStruct) + sizeof(TSS));
	
	// set the pointers of sub-structs
    ThreadStruct *thread = (ThreadStruct *)(task + 1);
	task->thread = thread;
	task->mem = (TaskMemStruct *)(thread + 1);
	task->tss = (TSS *)(task->mem + 1);
	// printk(WHITE, BLACK, "task=%#018lx ", task);

	task->flags = flag;
    task->vRunTime = 0;
    task->pid = Task_pidCounter++;
	// printk(WHITE, BLACK, "pid:%ld ", task->pid);
    task->mem->pgdPhyAddr = pgdPhyAddr;
	task->state = Task_State_Uninterruptible;

	memset(task->tss, 0, sizeof(TSS));
    // execption stack pointer
	task->tss->ist1	= Task_intrStackEnd;
    // interrupt stack pointer
	task->tss->ist2 = Task_intrStackEnd;
	task->tss->ist3 = task->tss->ist4 = task->tss->ist5 = task->tss->ist6 = task->tss->ist7 = Task_intrStackEnd;
	task->tss->rsp0 = task->tss->rsp1 = task->tss->rsp2 = Task_kernelStackEnd;

	task->vRunTime = ((SMP_current->flags & SMP_CPUInfo_flag_InTaskLoop) ? Task_current->vRunTime : 0);

    *thread = Init_thread;
    thread->rip = (u64)Task_kernelThreadEntry;
    thread->rbp = Task_kernelStackEnd;
    thread->rsp = Task_kernelStackEnd - sizeof(PtReg);
    thread->rsp3 = Task_userStackEnd;
	thread->fs = thread->gs = Segment_kernelData;

	// prepare the first 
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rflags = (1 << 9);
    regs.cs = Segment_kernelCode;
    regs.ds = Segment_kernelData;
	regs.es = Segment_kernelData;
	regs.ss = Segment_kernelData;
	regs.rsp = Task_kernelStackEnd;

	if (flag & Task_Flag_Kernel) {
		regs.rip = (u64)entry;
		regs.rdi = (u64)arg1;
		regs.rsi = arg2;
	} else {
		regs.rip = (u64)init;
		regs.rdi = (u64)entry;
		regs.rsi = (u64)_argWrap(arg1, arg2);
	}

	// construct page table and stack
    {
		// copy the kernel part (except stack) of pgd
		u64 *srcCR3 = DMAS_phys2Virt((flag & Task_Flag_Slaver) ? Task_current->mem->pgdPhyAddr : 0x101000);
        Page *lstPage;

		memcpy(srcCR3 + 256, (u64 *)DMAS_phys2Virt(pgdPhyAddr) + 256, 255 * sizeof(u64));
        *((u64 *)DMAS_phys2Virt(pgdPhyAddr) + 255) = 0;

		lstPage = task->mem->intrPage = MM_Buddy_alloc(5, Page_Flag_Active | Page_Flag_KernelShare);

		// map the interrupt stack with full present pages
		for (u64 vAddr = Task_intrStackEnd - Task_intrStackSize; vAddr < Task_intrStackEnd; vAddr += Page_4KSize)
			MM_PageTable_map(pgdPhyAddr, 
					vAddr, lstPage->phyAddr + (vAddr - (Task_intrStackEnd - Task_intrStackSize)), 
					MM_PageTable_Flag_Presented | MM_PageTable_Flag_Writable | MM_PageTable_Flag_UserPage);

		// map the kernel stack with full present pages
		for (u64 vAddr = Task_kernelStackEnd - 0xff0ul; vAddr >= Task_kernelStackEnd - Task_kernelStackSize; vAddr -= Page_4KSize)
			MM_PageTable_map(pgdPhyAddr,
					vAddr, tskStructPage->phyAddr + (vAddr - (Task_kernelStackEnd - Task_kernelStackSize)), 
					MM_PageTable_Flag_Writable | MM_PageTable_Flag_Presented);

		// copy the data into the stack
		memcpy(&regs, (u64 *)DMAS_phys2Virt(tskStructPage->phyAddr + Task_kernelStackSize - sizeof(PtReg)), sizeof(PtReg));
	}

    // initalize the usage information
    List_init(&task->mem->pageUsage);
	List_init(&task->mem->kmallocUsage);

	// set the signal handler
	Task_setSysSignalHandler(task);

	// set the timer tree
	RBTree_init(&task->timerTree, Task_Timer_comparator);
	SpinLock_init(&task->timerTreeLock);

	// initialize the simd structure
	task->simdRegs = SIMD_allocXsaveArea(Slab_kmalloc_arg_Clear, NULL);
	SIMD_kernelAreaStart;
	SIMD_xsave(task->simdRegs);
	SIMD_kernelAreaEnd;

	// choose a processor
	task->cpuId = task->pid % SMP_cpuNum;

	// insert the task into the cfs struct
    if (task->pid > 0) {
		int isCurCPU = (task->cpuId == SMP_getCurCPUIndex());
		if (isCurCPU) IO_cli();
		SpinLock_lock(&Task_cfsStruct.lock[task->cpuId]);
		RBTree_insNode(&Task_cfsStruct.tree[task->cpuId], &task->wNode);
		SpinLock_unlock(&Task_cfsStruct.lock[task->cpuId]);
		if (isCurCPU) IO_sti();
	}
    return task;
}

int Task_getRing() {
    u64 cs;
    __asm__ volatile ("movq %%cs, %0" : "=a"(cs));
    return cs & 3;
}

__always_inline__ void Task_kernelEntryHeader() {
	SMP_current->flags |= SMP_CPUInfo_flag_InTaskLoop;
	Task_current->state = Task_State_Running;
	IO_sti();
}

#define RecycleThread_State_Idle        0
#define RecycleThread_State_Scanning    1
#define RecycleThread_State_Running     2

/// @brief when the task is finished, this function will be executed to recycle the resource that this task used. (e.g. memory, ports)
void Task_exit() {
    for (List *pageList = Task_current->mem->pageUsage.next, *nxt = NULL; pageList != &Task_current->mem->pageUsage; pageList = nxt) {
        nxt = pageList->next;
        List_del(pageList);
        MM_Buddy_free(container(pageList, Page, listEle));
    }
	for (List *kmallocList = Task_current->mem->kmallocUsage.next, *nxt = NULL; kmallocList != &Task_current->mem->kmallocUsage; kmallocList = nxt) {
		nxt = kmallocList->next;
		printk(WHITE, BLACK, "Task_exit(): recycle SLAB memory %#018lx\n", container(kmallocList, Task_KmallocUsage, listEle)->addr);
		kfree(container(kmallocList, Task_KmallocUsage, listEle)->addr, Slab_kmalloc_arg_Private);
	}
    if (Task_current->mem->totUsage > 0) {
        printk(RED, BLACK, "Task_exit(): task %ld: failed to recycle all the page frame, remain %ld pages.\n", Task_current->pid, Task_current->mem->totUsage);
        while (1) IO_hlt();
    }
	// printk(WHITE, BLACK, "Task_exit(): %d\n", Task_current->pid);
	Task_current->priority = Task_Priority_Killed;

    while (1) IO_hlt();
}

void Task_recycleThread(void *arg1, u64 arg2) {
    Task_kernelEntryHeader();
    while (1) {
		while (Task_cfsStruct.killedTaskNum.value == 0) IO_hlt();
		Atomic_inc(&Task_cfsStruct.recycTskState);
        RBNode *rMost = RBTree_getMax(&Task_cfsStruct.killedTree);
        if (rMost != NULL) {
			TaskStruct *tsk = container(rMost, TaskStruct, wNode);
			if (!(tsk->flags & Task_Flag_InKillTree)) {
				Atomic_dec(&Task_cfsStruct.recycTskState);
				IO_hlt();
				continue;
			}
            RBTree_delNode(&Task_cfsStruct.killedTree, rMost);
			Atomic_dec(&Task_cfsStruct.killedTaskNum);
			Atomic_dec(&Task_cfsStruct.recycTskState);
            printk(BLACK, WHITE, "recycle task %d at %#018lx\n", tsk->pid, tsk);
            MM_Buddy_free(tsk->mem->intrPage);
            MM_PageTable_cleanMap(tsk->mem->pgdPhyAddr);
			kfree(tsk->simdRegs, 0); 
            // free the page of the task structure
            Page *tskPage = memManageStruct.pages + (DMAS_virt2Phys(tsk) >> Page_4KShift);
            MM_Buddy_free(tskPage);
        } else Atomic_dec(&Task_cfsStruct.recycTskState);
		IO_hlt();
    }
    Task_kernelThreadExit(0);
}

void Task_initMgr() {
	for (int i = 0; i < SMP_cpuNum; i++) {
		RBTree_init(&Task_cfsStruct.tree[i], _CFSTree_comparator);
		SpinLock_init(&Task_cfsStruct.lock[i]);
	}
    RBTree_init(&Task_cfsStruct.killedTree, _CFSTree_comparator);
    SpinLock_init(&Task_cfsStruct.killedTreeLock);

	// register the soft interrupt for schedule
	Intr_SoftIrq_register(1, Task_SoftIrq_testSchedule, NULL);

	Task_cfsStruct.flags = 1;
	Task_cfsStruct.killedTaskNum.value = 0;
	Task_cfsStruct.recycTskState.value = 0;
}