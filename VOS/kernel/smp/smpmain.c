#include "../includes/smp.h"
#include "../includes/log.h"

void startSMP() {
	u64 rsp = 0;
	SMP_CPUInfoPkg *pkg = SMP_current;
	rsp = (u64)pkg->initStk + Init_taskStackSize;	
	u32 x, y;
	__asm__ volatile (
		"movq $0x1b, %%rcx		\n\t"
		"rdmsr 					\n\t"
		"bts $10, %%rax			\n\t"
		"bts $11, %%rax			\n\t"
		"wrmsr					\n\t"
		"movq $0x1b, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory", "rcx"
	);

	__asm__ volatile (
		"movq $0x80f, %%rcx		\n\t"
		"rdmsr					\n\t"
		"bts $8, %%rax			\n\t"
		"bts $12, %%rax			\n\t"
		"wrmsr					\n\t"
		"movq $0x80f, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory", "rcx"
	);
	__asm__ volatile (
		"movq $0x802, %%rcx		\n\t"
		"rdmsr					\n\t"
		: "=a"(x), "=d"(y)
		:
		: "memory"
	);
	// half of the initStk to be the trap stack
	rsp -= 0x4000ul;
	Intr_Gate_setTSS(pkg->tssTable, rsp + 0x4000ul, rsp + 0x4000ul, rsp + 0x4000ul, rsp, rsp, rsp, rsp, rsp, rsp, rsp);
	Intr_Gate_loadTR(pkg->trIdx);
	printk(WHITE, BLACK, "APU %d: tr:%d trap rsp:%#018lx\n", SMP_getCurCPUIndex(), pkg->trIdx, rsp);
	IO_sti();
	SMP_current->flags |= SMP_CPUInfo_flag_APUInited;

	// wait for the first task, and then jump to it
	int idx = SMP_getCurCPUIndex();
	TaskStruct *task;
	while (1) {
		SpinLock_lock(&Task_cfsStruct.lock[idx]);
		RBNode *leftMost = RBTree_getMin(&Task_cfsStruct.tree[idx]);
		if (leftMost) {
			task = container(leftMost, TaskStruct, wNode);
			RBTree_delNode(&Task_cfsStruct.tree[idx], leftMost);
			break;
		}
		SpinLock_unlock(&Task_cfsStruct.lock[idx]);
	}
	SpinLock_unlock(&Task_cfsStruct.lock[idx]);
	
	Task_switch_init(NULL, task);
	while (1) IO_hlt();
}