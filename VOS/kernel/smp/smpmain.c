#include "../includes/smp.h"
#include "../includes/simd.h"
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
	IO_sti();
	SMP_current->flags |= SMP_CPUInfo_flag_APUInited;
	Task_Syscall_init();

	while (!Task_cfsStruct.flags) ;

	// wait for the first task, and then jump to it
	int idx = SMP_getCurCPUIndex();
	while (!Task_cfsStruct.taskNum[idx].value) ;
	RBNode *node = RBTree_getMin(&Task_cfsStruct.tree[idx]);
	RBTree_delNode(&Task_cfsStruct.tree[idx], node);
	TaskStruct *task = container(node, TaskStruct, wNode);
	SIMD_enable();
	SIMD_setTS();
	Task_switch_init(NULL, task);
	while (1) IO_hlt();
}