#include "syscall.h"
#include "mgr.h"
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

void Syscall_entry();
void Syscall_exit();

u64 Syscall_noSystemCall(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    printk(WHITE, BLACK, "no such system call, arg1 = %#018lx, arg2 = %#018lx, arg3 = %#018lx, arg4 = %#018lx, arg5 = %#018lx\n",
        arg1, arg2, arg3, arg4, arg5);
    return 1919810;
}

u64 Syscall_enableIntr(u64 intrId, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    HW_APIC_enableIntr(intrId);
    return 0;
}

u64 Syscall_divZero(u64 intrId, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
	int i = 1 / 0;
	return 0;
}

u64 Syscall_abort(u64 intrId, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    int t = 1000000000;
    while (t--) ;
    return 0;
}

typedef u64 (*Syscall)(u64, u64, u64, u64, u64);
Syscall Syscall_list[Syscall_num] = { 
    [0] = Syscall_abort,
    [1] = Syscall_printStr,
	[2] = Syscall_divZero,
	[3] = Syscall_noSystemCall,
    [4 ... Syscall_num - 1] = Syscall_noSystemCall };

u64 Syscall_handler(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    // switch stack and segment registers
    Task_current->tss->rsp0 = Task_current->thread->rsp0;
    Intr_Gate_setTSS(
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
	IO_sti();
    u64 res = (Syscall_list[index])(arg1, arg2, arg3, arg4, arg5);
    // switch to user level
    return res;
}

u64 Task_Syscall_usrAPI(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    // not necessary to switch the stack
    // directly use "syscall"
    u64 res;
    __asm__ volatile (
        "movq %1, %%rdi \n\t"
        "movq %2, %%rsi \n\t"
        "movq %3, %%rdx \n\t"
        "movq %4, %%rax \n\t"
        "movq %5, %%r8 \n\t"
        "movq %6, %%r9  \n\t"
        "syscall        \n\t"
        "movq %%rax, %0 \n\t"
         : "=m"(res) 
         : "m"(index), "m"(arg1), "m"(arg2), "m"(arg3), "m"(arg4), "m"(arg5)
         : "memory");
    return res;
}


void Task_switchToUsr(u64 (*entry)(u64), u64 arg) {
	IO_cli();
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    printk(RED, BLACK, "Task_switchToUsr: entry = %#018lx, arg = %#018lx\t", entry, arg);
    printk(WHITE, BLACK, "pid = %ld\n", Task_current->pid);
    regs.rsp = Task_userStackEnd;
    regs.rdi = arg;
    regs.rcx = (u64)entry;
    regs.r11 = (1 << 9);
    regs.cs = Segment_userCode;
    regs.ds = Segment_userData;
    regs.es = Segment_userData;
    regs.ss = Segment_userData;
    __asm__ volatile (
        "subq %1, %%rsp     \n\t"
		"movq %%rsp, %0     \n\t"
        : "=m"(Task_current->thread->rsp0)
        : "a"(sizeof(PtReg))
        :
    );
    Task_current->thread->rsp = Task_current->thread->rsp3 = regs.rsp;
    memcpy(&regs, (void *)(Task_current->thread->rsp0), sizeof(PtReg));
    Task_current->tss->rsp0 = Task_current->thread->rsp0;
    Intr_Gate_setTSS(
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
    __asm__ volatile (
        "movq %0, %%rsp     \n\t"
        "jmp Syscall_exit	\n\t"
        :
        : "m"(Task_current->thread->rsp0)
        : "memory"
    );
}

void Task_Syscall_init() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xC0000080, IO_readMSR(0xC0000080) | 1);
	printk(GREEN, BLACK, "write 0xC0000080 -> %lx\t", IO_readMSR(0xC0000080));
    // set IA32_STAR
    IO_writeMSR(0xC0000081, ((u64)(0x28 | 3) << 48) | ((u64)0x8 << 32));
	printk(GREEN, BLACK, "write 0xC0000081 -> %lx\t", IO_readMSR(0xC0000081));
    // set IA32_LSTAR
    IO_writeMSR(0xC0000082, (u64)Syscall_entry);
	printk(GREEN, BLACK, "write 0xC0000082 -> %lx\t", IO_readMSR(0xC0000082));
    // set IA32_SFMASK
    IO_writeMSR(0xC0000084, (1 << 9));
	printk(GREEN, BLACK, "write 0xC0000084 -> %lx\n", IO_readMSR(0xC0000084));
}
