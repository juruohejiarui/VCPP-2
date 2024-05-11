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
    APIC_enableIntr(intrId);
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
    [3 ... Syscall_num - 1] = Syscall_noSystemCall };

u64 Syscall_handler(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    // switch stack and segment registers
    Task_current->tss->rsp0 = Task_current->thread->rsp0;
    Gate_setTSS(
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
    u64 res = (Syscall_list[index])(arg1, arg2, arg3, arg4, arg5);
    // switch to user level
    return res;
}

u64 Syscall_usrAPI(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
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


void Task_switchToUsr(u64 (*entry)(), u64 arg) {
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    printk(RED, BLACK, "Task_switchToUsr: entry = %#018lx, arg = %#018lx\n", entry, arg);
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
	printk(YELLOW, BLACK, "finished copying\n");
    Task_current->tss->rsp0 = Task_current->thread->rsp0;
    Gate_setTSS(
        Task_current->tss->rsp0, Task_current->tss->rsp1, Task_current->tss->rsp2, Task_current->tss->ist1, Task_current->tss->ist2,
		Task_current->tss->ist3, Task_current->tss->ist4, Task_current->tss->ist5, Task_current->tss->ist6, Task_current->tss->ist7);
    printk(ORANGE, BLACK, "finish TSS\n");
    __asm__ volatile (
        "movq %0, %%rsp     \n\t"
        "jmp Syscall_exit     \n\t"
        :
        : "m"(Task_current->thread->rsp0)
        : "memory"
    );
}

u64 Task_initUsrLevel(u64 arg) {
	Task_current->state = Task_State_Running;
    printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    u64 res = Syscall_usrAPI(arg, BLACK, WHITE, (u64)"Up Down Up Down baba", 20, 5);
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    int t = 10000000 * (arg + 3), initCounter = 100000000;
    int tmp = arg;
    while (tmp > 0) initCounter <<= 1, tmp--;
    if (arg == 1) initCounter = t = 1;
    while (1) {
        if (arg == 1) Syscall_usrAPI(0, 0x14, 2, 3, 4, 5);
        (--t == 0 ? printk(WHITE, BLACK, "%d ", Task_current->pid), t = initCounter : 0);
    }
    while (1) ;
}

void Init_syscall() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xC0000080, IO_readMSR(0xC0000080) | 1);
    // set IA32_STAR
    IO_writeMSR(0xC0000081, ((u64)0x28 << 48) | ((u64)0x8 << 32));
    // set IA32_LSTAR
    IO_writeMSR(0xC0000082, (u64)Syscall_entry);
    // set IA32_FMASK
    IO_writeMSR(0xC0000084, 0x00);
    // set IA32_SFMASK
    IO_writeMSR(0xC0000085, 0x00);
}
