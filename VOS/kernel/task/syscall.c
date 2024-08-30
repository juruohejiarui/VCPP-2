#include "syscall.h"
#include "mgr.h"
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"
#include "../includes/smp.h"

void Syscall_entry();
void Syscall_exit();

u64 Syscall_noSystemCall(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    printk(WHITE, BLACK, "no such system call, args:(%#018lx,%#018lx,%#018lx,%#018lx,%#018lx)\n",
        arg1, arg2, arg3, arg4, arg5, arg6);
    return 1919810;
}

u64 Syscall_enableIntr(u64 intrId) {
    HW_APIC_enableIntr(intrId);
    return 0;
}

u64 Syscall_abort(u64 intrId) {
    int t = 1000000000;
    while (t--) ;
    return 0;
}

u64 Syscall_mdelay(u64 msec) {
    if (!(IO_getRflags() & (1 << 9))) {
        printk(RED, BLACK, "interrupt is blocked, unable to execute mdelay()\n");
        return -1;
    }
    Intr_SoftIrq_Timer_mdelay(msec);
    return 0;
}

typedef u64 (*Syscall)(u64, u64, u64, u64, u64, u64);
Syscall Syscall_list[Syscall_num] = { 
    [0] = (Syscall)Syscall_abort,
    [1] = (Syscall)Syscall_printStr,
	[2] = (Syscall)Syscall_mdelay,
    [3 ... Syscall_num - 1] = Syscall_noSystemCall };

u64 Syscall_handler(u64 index, PtReg *regs) {
    u64 arg1 = regs->rdi, arg2 = regs->rsi, arg3 = regs->rdx, arg4 = regs->rcx, arg5 = regs->r8, arg6 = regs->r9;
    // switch stack and segment registers
    Task_current->tss->rsp0 = Task_current->thread->rsp;
    Intr_Gate_setTSS(
        SMP_cpuInfo[Task_current->cpuId].tssTable,
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
	IO_sti();
    u64 res = (Syscall_list[index])(arg1, arg2, arg3, arg4, arg5, arg6);
    // switch to user level
	IO_cli();
    return res;
}

u64 Task_Syscall_usrAPI(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5, u64 arg6) {
    // directly use "syscall"
    u64 res;
    PtReg regs;
    regs.rdi = arg1, regs.rsi = arg2;
    regs.rdx = arg3, regs.rcx = arg4;
    regs.r8 = arg5, regs.r9 = arg6;
    __asm__ volatile (
        "syscall        \n\t"
        "movq %%rax, %0 \n\t"
         : "=m"(res) 
         : "D"(index), "S"((u64)&regs)
         : "memory", "rax");
    return res;
}


void Task_switchToUsr(u64 (*entry)(void *, u64), void *arg1, u64 arg2) {
	IO_cli();
	u64 rsp = 0;
	__asm__ volatile (
		"movq %%rsp, %0		\n\t"
		: "=m"(rsp)
		:
		: "memory"
	);
	rsp -= 0x30;
    Intr_Gate_setTSS(
        SMP_current->tssTable,
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
	*(u64 *)(rsp + 0x00) = Task_userStackEnd;
	*(u64 *)(rsp + 0x08) = Task_userStackEnd;
	*(u64 *)(rsp + 0x10) = (1 << 9);
	*(u64 *)(rsp + 0x18) = (u64)entry;
	*(u64 *)(rsp + 0x20) = Segment_userData;
	*(u64 *)(rsp + 0x28) = Segment_userData;
    __asm__ volatile (
		"movq %0, %%rsp		\n\t"
        "jmp Syscall_exit	\n\t"
        :
        : "a"(rsp), "D"(arg1), "S"(arg2)
        : "memory"
    );
}

void Task_Syscall_init() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xC0000080, IO_readMSR(0xC0000080) | 1);
    // set IA32_STAR
    IO_writeMSR(0xC0000081, ((u64)(0x28 | 3) << 48) | ((u64)0x8 << 32));
    // set IA32_LSTAR
    IO_writeMSR(0xC0000082, (u64)Syscall_entry);
    // set IA32_SFMASK
    IO_writeMSR(0xC0000084, (1 << 9));
}
